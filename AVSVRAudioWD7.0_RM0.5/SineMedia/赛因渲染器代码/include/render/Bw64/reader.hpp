/// @file reader.hpp
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
#include "parser.hpp"

#include <iostream>

namespace bw64 {

  /**
   * @brief Representation of a BW64 file
   *
   * 在此使用bw64:：readFile（）创建bw64的实例.
   *
   * 此文件将在构造时打开和初始化，进行解析头、格式等，并在解构时关闭。
   * 
   */
  class Bw64Reader {
   public:
    /**
     * @brief Open a new BW64 file for reading
     *
     * 打开一个新的BW64文件进行读取，解析整个文件以读取格式并识别其中的所有块。
     * 为了方便，考虑使用“Readfile`帮助函数。
     * 
     */
    Bw64Reader(const char* filename) {
      fileStream_.open(filename, std::fstream::in | std::fstream::binary);
      if (!fileStream_.is_open()) {
        std::stringstream errorString;
        errorString << "Could not open file: " << filename;
        throw std::runtime_error(errorString.str());
      }
      readRiffChunk();
      if (fileFormat_ == utils::fourCC("BW64") ||
          fileFormat_ == utils::fourCC("RF64")) {
        auto chunkHeader = parseHeader();
        if (chunkHeader.id != utils::fourCC("ds64")) {
          throw std::runtime_error(
              "mandatory ds64 chunk for BW64 or RF64 file not found");
        }
        auto ds64Chunk =
            parseDataSize64Chunk(fileStream_, chunkHeader.id, chunkHeader.size);
        chunks_.push_back(ds64Chunk);
        chunkHeaders_.push_back(chunkHeader);
      }
      parseChunkHeaders();
      for (auto chunkHeader : chunkHeaders_) {
        if (chunkHeader.id != utils::fourCC("ds64")) {
          auto chunk = parseChunk(fileStream_, chunkHeader);
          chunks_.push_back(chunk);
        }
      }

      auto fmtChunk = formatChunk();
      if (!fmtChunk) {
        throw std::runtime_error("mandatory fmt chunk not found");
      }
      channelCount_ = fmtChunk->channelCount();
      formatTag_ = fmtChunk->formatTag();
      sampleRate_ = fmtChunk->sampleRate();
      bitsPerSample_ = fmtChunk->bitsPerSample();

      seek(0);
    }

    /**
     * 
     *
     * 解构函数将自动关闭在构造函数中打开的文件。
     * 
     */
    ~Bw64Reader() { fileStream_.close(); }

    /// @brief 获取文件格式(RIFF, BW64或RF64)
    uint32_t fileFormat() const { return fileFormat_; }
    /// @brief 获取文件大小值
    uint32_t fileSize() const { return fileSize_; }
    /// @brief 获取格式标签
    uint16_t formatTag() const { return formatTag_; };
    /// @brief 获取通道数量
    uint16_t channels() const { return channelCount_; };
    /// @brief 获取取样率
    uint32_t sampleRate() const { return sampleRate_; };
    /// @brief 获取位深
    uint16_t bitDepth() const { return bitsPerSample_; };
    /// @brief 获得帧数
    uint64_t numberOfFrames() const {
      return dataChunk()->size() / blockAlignment();
    }
    /// @brief 获取数据块对齐
    uint16_t blockAlignment() const { return channels() * bitDepth() / 8; }

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

    /**
     * @brief 获取“ds64” chunk
     *
     * @returns 返回值，用于DataSize64Chunk。
     */
    std::shared_ptr<DataSize64Chunk> ds64Chunk() const {
      return chunk<DataSize64Chunk>(chunks_, utils::fourCC("ds64"));
    }
    /**
     * @brief 获取格式chunk
     *
     * @returns 返回值，用于FormatInfoChunk
     */
    std::shared_ptr<FormatInfoChunk> formatChunk() const {
      return chunk<FormatInfoChunk>(chunks_, utils::fourCC("fmt "));
    }
    /**
     * @brief 获取“data” chunk
     *
     * @warning 因为对数据块的访问由Bw64Reader类单独处理，通常调用此方法。
     *
     * @returns 返回值，用于DataChunk。
     */
    std::shared_ptr<DataChunk> dataChunk() const {
      return chunk<DataChunk>(chunks_, utils::fourCC("data"));
    }
    /**
     * @brief 获取 'chna' chunk
     *
     * @returns 返回值，用于ChnaChunk。
     */
    std::shared_ptr<ChnaChunk> chnaChunk() const {
      return chunk<ChnaChunk>(chunks_, utils::fourCC("chna"));
    }
    /**
     * @brief 获取 'axml' chunk
     *
     * @returns 返回值，用于AxmlChunk。
     */
    std::shared_ptr<AxmlChunk> axmlChunk() const {
      return chunk<AxmlChunk>(chunks_, utils::fourCC("axml"));
    }

    /**
     * @brief 获取文件中存在的所有chunks的列表
     */
    std::vector<ChunkHeader> chunks() const { return chunkHeaders_; }

    /**
     * @brief 检查是否存在具有给定id的chunk
     */
    bool hasChunk(uint32_t id) const {
      auto foundHeader = std::find_if(
          chunkHeaders_.begin(), chunkHeaders_.end(),
          [id](const ChunkHeader header) { return header.id == id; });
      if (foundHeader == chunkHeaders_.end()) {
        return false;
      } else {
        return true;
      }
    }

    /**
     * @brief 在数据块中跳过帧位置
     */
    void seek(int32_t offset, std::ios_base::seekdir way = std::ios::beg) {
      const int64_t frameOffset =
          offset * static_cast<int64_t>(formatChunk()->blockAlignment());
      const int64_t chunkPosition =
          getChunkHeader(utils::fourCC("data")).position + 8;
      int64_t dataChunkOffset = 0;
      if (way == std::ios::cur) {
        dataChunkOffset = fileStream_.tellg();
      } else if (way == std::ios::beg) {
        dataChunkOffset = chunkPosition;
      } else if (way == std::ios::end) {
        dataChunkOffset = chunkPosition + dataChunk()->size();
      }
      fileStream_.clear();
      if (frameOffset < 0 && dataChunkOffset < -frameOffset) {
        fileStream_.seekg(0);
      } else {
        fileStream_.seekg(dataChunkOffset + frameOffset);
      }
      const int64_t fileStreamPos = fileStream_.tellg();
      if (fileStreamPos < chunkPosition) {
        fileStream_.seekg(chunkPosition);
      } else if (fileStreamPos >
                 chunkPosition + static_cast<int64_t>(dataChunk()->size())) {
        fileStream_.seekg(chunkPosition + dataChunk()->size());
      }
    }

    /**
     * @brief 从dataChunk里读取数据帧
     *
     * @param[out] 使用outBuffer建立将样本写入数据帧的缓存
     * @param[in]  将要读取的数据帧数量
     *
     * @returns 返回值，用于读取的帧数量。
     */
    template <typename T,
              typename = std::enable_if<std::is_floating_point<T>::value>>
    uint64_t read(T* outBuffer, uint64_t frames) {
      if (tell() + frames > numberOfFrames()) {
        frames = numberOfFrames() - tell();
      }
      rawDataBuffer_.resize(frames * blockAlignment());
      fileStream_.read(&rawDataBuffer_[0], frames * blockAlignment());
      utils::decodePcmSamples(&rawDataBuffer_[0], outBuffer,
                              frames * channels(), bitDepth());
      return frames;
    }

    /**
     * @brief 指出dataChunk的当前帧位置
     *
     * @returns 返回dataChunk的当前帧位置 
     */
    uint64_t tell() {
      return ((uint64_t)fileStream_.tellg() -
              getChunkHeader(utils::fourCC("data")).position - 8u) /
             formatChunk()->blockAlignment();
    }

    /**
     * @brief 检查是否到达数据末端
     *
     * @returns 当到达数据末端时返回值为`true` ，否则 `false`
     */
    bool eof() { return tell() == numberOfFrames(); }

   private:
    void readRiffChunk() {
      uint32_t riffType;
      utils::readValue(fileStream_, fileFormat_);
      utils::readValue(fileStream_, fileSize_);
      utils::readValue(fileStream_, riffType);

      if (fileFormat_ != utils::fourCC("RIFF") &&
          fileFormat_ != utils::fourCC("BW64") &&
          fileFormat_ != utils::fourCC("RF64")) {
        throw std::runtime_error("File is not a RIFF, BW64 or RF64 file.");
      }
      if (riffType != utils::fourCC("WAVE")) {
        throw std::runtime_error("File is not a WAVE file.");
      }
    }

    ChunkHeader getChunkHeader(uint32_t id) {
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

    ChunkHeader parseHeader() {
      uint32_t chunkId;
      uint32_t chunkSize;
      uint64_t position = fileStream_.tellg();
      utils::readValue(fileStream_, chunkId);
      utils::readValue(fileStream_, chunkSize);
      uint64_t chunkSize64 = getChunkSize64(chunkId, chunkSize);
      return ChunkHeader(chunkId, chunkSize64, position);
    }

    uint64_t getChunkSize64(uint32_t id, uint64_t chunkSize) {
      if (ds64Chunk()) {
        if (id == utils::fourCC("BW64") || id == utils::fourCC("RF64")) {
          return ds64Chunk()->bw64Size();
        }
        if (id == utils::fourCC("data")) {
          return ds64Chunk()->dataSize();
        }
        if (ds64Chunk()->hasChunkSize(id)) {
          return ds64Chunk()->getChunkSize(id);
        }
      }
      return chunkSize;
    }

    void parseChunkHeaders() {
      while (fileStream_.peek() != EOF) {
        auto chunkHeader = parseHeader();
        chunkHeader.size = getChunkSize64(chunkHeader.id, chunkHeader.size);
        chunkHeaders_.push_back(chunkHeader);
        if (chunkHeader.size % 2 == 0) {
          fileStream_.seekg(chunkHeader.size, std::ios::cur);
        } else {
          // 查找字节填充
          fileStream_.seekg(chunkHeader.size + 1, std::ios::cur);
        }
      }
    }

    std::ifstream fileStream_;
    uint32_t fileFormat_;
    uint32_t fileSize_;
    uint16_t channelCount_;
    uint32_t sampleRate_;
    uint16_t formatTag_;
    uint16_t bitsPerSample_;

    std::vector<char> rawDataBuffer_;
    std::vector<std::shared_ptr<Chunk>> chunks_;
    std::vector<ChunkHeader> chunkHeaders_;
  };
}  // namespace bw64
