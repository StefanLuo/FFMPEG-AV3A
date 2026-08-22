/// @file writer.hpp
#pragma once
#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>
#include "chunks.hpp"
#include "utils.hpp"

namespace bw64 {

  const uint32_t MAX_NUMBER_OF_UIDS = 1024;

  /**
   * @brief BW64 写入器类
   *
   * 使用bw64:：writeFile（）创建此类的实例。
   *
   * 这是一个[RAII]类，文件将在构建时打开并初始化，在解构时关闭并最终确定（写入块大小等）。
   */
  class Bw64Writer {
   public:
    /**
     * @brief 打开新的BW64文件进行写入
     *
     * 打开一个新的BW64文件进行写入，初始化“数据”块中的所有内容。
     * 然后将交错的音频样本写入此文件。
     *
     * @warning 如果该文件已经存在，它将被覆盖。
     *
     * 如果需要在数据块之前显示任何chunk，包含在“additionalChunks”中。
     * chunk将在打开文件后直接写入。
     * @note 为了方便，可以使用“写入文件”helper函数。
     */
    Bw64Writer(const char* filename, uint16_t channels, uint32_t sampleRate,
               uint16_t bitDepth,
               std::vector<std::shared_ptr<Chunk>> additionalChunks) {
      fileStream_.open(filename, std::fstream::out | std::fstream::binary);
      if (!fileStream_.is_open()) {
        std::stringstream errorString;
        errorString << "Could not open file: " << filename;
        throw std::runtime_error(errorString.str());
      }
      writeRiffHeader();
      writeChunkPlaceholder(utils::fourCC("JUNK"), 28u);
      auto formatChunk =
          std::make_shared<FormatInfoChunk>(channels, sampleRate, bitDepth);
      writeChunk(formatChunk);

      for (auto chunk : additionalChunks) {
        writeChunk(chunk);
      }
      if (!chnaChunk()) {
        writeChunkPlaceholder(utils::fourCC("chna"),
                              MAX_NUMBER_OF_UIDS * 40 + 4);
      }
      auto dataChunk = std::make_shared<DataChunk>();
      writeChunk(dataChunk);
    }

    /**
     * @brief 最终确定文件
     *
     * 此函数将把所有尚未写入的chunk写入文件，并最终确定所有必需的信息，即最终的chunk大小等。
     */
    ~Bw64Writer() {
      finalizeDataChunk();
      for (auto chunk : postDataChunks_) {
        writeChunk(chunk);
      }
      finalizeRiffChunk();
      fileStream_.close();
    }

    /// @brief 获取格式标签
    uint16_t formatTag() const { return formatChunk()->formatTag(); };
    /// @brief 获取通道处
    uint16_t channels() const { return formatChunk()->channelCount(); };
    /// @brief 获取取样率
    uint32_t sampleRate() const { return formatChunk()->sampleRate(); };
    /// @brief 获取位深
    uint16_t bitDepth() const { return formatChunk()->bitsPerSample(); };
    /// @brief 获取帧数量
    uint64_t framesWritten() const {
      return dataChunk()->size() / formatChunk()->blockAlignment();
    }

    template <typename ChunkType>
    std::vector<std::shared_ptr<ChunkType>> chunksWithId(
        const std::vector<Chunk>& chunks, uint32_t chunkId) const {
      std::vector<char> foundChunks;
      auto chunk =
          std::copy_if(chunks.begin(), chunks.end(), foundChunks.begin(),
                       [chunkId](const std::shared_ptr<Chunk> chunk) {
                         return chunk->id() == chunkId;
                       });
      return foundChunks;
    }

    template <typename ChunkType>
    std::shared_ptr<ChunkType> chunk(
        const std::vector<std::shared_ptr<Chunk>>& chunks,
        uint32_t chunkId) const {
      auto chunk = std::find_if(chunks.begin(), chunks.end(),
                                [chunkId](const std::shared_ptr<Chunk> chunk) {
                                  return chunk->id() == chunkId;
                                });
      if (chunk != chunks.end()) {
        return std::static_pointer_cast<ChunkType>(*chunk);
      } else {
        return nullptr;
      }
    }

    std::shared_ptr<DataSize64Chunk> ds64Chunk() const {
      return chunk<DataSize64Chunk>(chunks_, utils::fourCC("ds64"));
    }
    std::shared_ptr<FormatInfoChunk> formatChunk() const {
      return chunk<FormatInfoChunk>(chunks_, utils::fourCC("fmt "));
    }
    std::shared_ptr<DataChunk> dataChunk() const {
      return chunk<DataChunk>(chunks_, utils::fourCC("data"));
    }
    std::shared_ptr<ChnaChunk> chnaChunk() const {
      return chunk<ChnaChunk>(chunks_, utils::fourCC("chna"));
    }
    std::shared_ptr<AxmlChunk> axmlChunk() const {
      return chunk<AxmlChunk>(chunks_, utils::fourCC("axml"));
    }

    /// @brief 检查文件是否大于4GB，从而判断BW64文件
    bool isBw64File() {
      if (riffChunkSize() > UINT32_MAX) {
        return true;
      }
      if (dataChunk()->size() > UINT32_MAX) {
        return true;
      }
      return false;
    }

    void setChnaChunk(std::shared_ptr<ChnaChunk> chunk) {
      if (chunk->numUids() > 1024) {
        // 判断：是否将前数据块chna chunk设置为JUNK chunk，并将chnaChunk添加到后数据块中？
        
        throw std::runtime_error("number of trackUids is > 1024");
      }
      auto last_position = fileStream_.tellp();
      overwriteChunk(utils::fourCC("chna"), chunk);
      fileStream_.seekp(last_position);
    }

    void setAxmlChunk(std::shared_ptr<Chunk> chunk) {
      postDataChunks_.push_back(chunk);
    }

    // @brief 为头文件获取chunk大小
    uint32_t chunkSizeForHeader(uint32_t id) {
      if (chunkHeader(id).size >= UINT32_MAX) {
        return UINT32_MAX;
      } else {
        return static_cast<uint32_t>(chunkHeader(id).size);
      }
    }

    /// @brief 计算riff chunk大小
    uint64_t riffChunkSize() {
      auto last_position = fileStream_.tellp();
      fileStream_.seekp(0, std::ios::end);
      uint64_t endPos = fileStream_.tellp();
      fileStream_.seekp(last_position);
      return endPos - 8u;
    }

    /// @brief 写RIFF文件头
    void writeRiffHeader() {
      uint32_t RiffId = utils::fourCC("RIFF");
      uint32_t fileSize = UINT32_MAX;
      uint32_t WaveId = utils::fourCC("WAVE");
      utils::writeValue(fileStream_, RiffId);
      utils::writeValue(fileStream_, fileSize);
      utils::writeValue(fileStream_, WaveId);
    }

    /// @brief 更新RIFF文件头
    void finalizeRiffChunk() {
      auto last_position = fileStream_.tellp();
      fileStream_.seekp(0);
      if (isBw64File()) {
        utils::writeValue(fileStream_, utils::fourCC("BW64"));
        utils::writeValue(fileStream_, INT32_MAX);
        overwriteJunkWithDs64Chunk();
      } else {
        utils::writeValue(fileStream_, utils::fourCC("RIFF"));
        uint32_t fileSize = static_cast<uint32_t>(riffChunkSize());
        utils::writeValue(fileStream_, fileSize);
      }
      fileStream_.seekp(last_position);
    }

    void overwriteJunkWithDs64Chunk() {
      auto ds64Chunk = std::make_shared<DataSize64Chunk>();
      ds64Chunk->bw64Size(riffChunkSize());
      ds64Chunk->dataSize(dataChunk()->size());
      // 增添另外的大于4GB的chunks
      overwriteChunk(utils::fourCC("JUNK"), ds64Chunk);
    }

    void finalizeDataChunk() {
      if (dataChunk()->size() % 2 == 1) {
        utils::writeValue(fileStream_, '\0');
      }
      auto last_position = fileStream_.tellp();
      seekChunk(utils::fourCC("data"));
      utils::writeValue(fileStream_, utils::fourCC("data"));
      utils::writeValue(fileStream_, chunkSizeForHeader(utils::fourCC("data")));
      fileStream_.seekp(last_position);
    }

    /// @brief 写入chunk模板
    template <typename ChunkType>
    void writeChunk(std::shared_ptr<ChunkType> chunk) {
      if (chunk) {
        uint64_t position = fileStream_.tellp();
        chunkHeaders_.push_back(
            ChunkHeader(chunk->id(), chunk->size(), position));
        utils::writeChunk<ChunkType>(fileStream_, chunk,
                                     chunkSizeForHeader(chunk->id()));
        chunks_.push_back(chunk);
      }
    }

    void writeChunkPlaceholder(uint32_t id, uint32_t size) {
      uint64_t position = fileStream_.tellp();
      chunkHeaders_.push_back(ChunkHeader(id, size, position));
      utils::writeChunkPlaceholder(fileStream_, id, size);
    }

    /// @brief 复写chunk模板
    template <typename ChunkType>
    void overwriteChunk(uint32_t id, std::shared_ptr<ChunkType> chunk) {
      auto last_position = fileStream_.tellp();
      seekChunk(id);
      utils::writeChunk<ChunkType>(fileStream_, chunk, chunkSizeForHeader(id));
      fileStream_.seekp(last_position);
    }

    void seekChunk(uint32_t id) {
      auto header = chunkHeader(id);
      fileStream_.clear();
      fileStream_.seekp(header.position);
    }

    ChunkHeader& chunkHeader(uint32_t id) {
      auto foundHeader = std::find_if(
          chunkHeaders_.begin(), chunkHeaders_.end(),
          [id](const ChunkHeader header) { return header.id == id; });
      if (foundHeader != chunkHeaders_.end()) {
        return *foundHeader;
      }
      std::stringstream errorMsg;
      errorMsg << "no chunk with id '" << utils::fourCCToStr(id) << "' found";
      throw std::runtime_error(errorMsg.str());
    }

    /**
     * @brief 将数据帧写入dataChunk
     *
     * @param[out] inBuffer：为写入的样本设置缓存
     * @param[in]  frames：设置要写入的数据帧数量
     *
     * @returns 返回要写入的数据帧数量
     */
    template <typename T,
              typename = std::enable_if<std::is_floating_point<T>::value>>
    uint64_t write(T* inBuffer, uint64_t frames) {
      uint64_t bytesWritten = frames * formatChunk()->blockAlignment();
      rawDataBuffer_.resize(bytesWritten);
      utils::encodePcmSamples(inBuffer, &rawDataBuffer_[0],
                              frames * formatChunk()->channelCount(),
                              formatChunk()->bitsPerSample());
      fileStream_.write(&rawDataBuffer_[0], bytesWritten);
      dataChunk()->setSize(dataChunk()->size() + bytesWritten);
      chunkHeader(utils::fourCC("data")).size = dataChunk()->size();
      return frames;
    }

   private:
    std::ofstream fileStream_;
    std::vector<char> rawDataBuffer_;
    std::vector<std::shared_ptr<Chunk>> chunks_;
    std::vector<ChunkHeader> chunkHeaders_;
    std::vector<std::shared_ptr<Chunk>> postDataChunks_;
  };

}  // namespace bw64
