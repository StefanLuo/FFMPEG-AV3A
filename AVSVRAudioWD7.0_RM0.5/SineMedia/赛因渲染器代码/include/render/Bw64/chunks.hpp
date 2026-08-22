/// @file chunks.hpp
#pragma once
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>
#include "utils.hpp"

namespace bw64 {

  /**
   * @brief 缓存chunks位置和大小的数据结构
   *
   * 打开文件和进行基本检查，扫描文件中的chunks。找到的chunks的数据（chunkId、chunkSize、文件中的位置）
   * 将数据保存在ChunkHeader对象中。
   */
  struct ChunkHeader {
    ChunkHeader(uint32_t id = 0, uint64_t size = 0, uint64_t position = 0)
        : id(id), size(size), position(position) {}
    uint32_t id;
    uint64_t size;
    uint64_t position;
  };

  /**
   * @brief RIFF chunk 基本分类
   */
  class Chunk {
   public:
    virtual ~Chunk() = default;

    /// @brief 获取FourCC id
    virtual uint32_t id() const = 0;
    /// @brief 获取chunk大小
    virtual uint64_t size() const = 0;

    /// @brief 将chunk写入流（stream）
    virtual void write(std::ostream& stream) const = 0;

   protected:
    Chunk() = default;
  };

  /**
   * @brief 自定义chunk的类
   *
   * 此类可用于将未知chunk从输入文件复制到输出文件。
   */
  class UnknownChunk : public Chunk {
   public:
    UnknownChunk(uint32_t id) { chunkId_ = id; }
    UnknownChunk(std::istream& stream, uint32_t id, uint64_t size) {
      chunkId_ = id;
      data_.resize(size);
      stream.read(&data_[0], size);
    }

    uint32_t id() const override { return chunkId_; }
    uint64_t size() const override { return data_.size(); }

    void write(std::ostream& stream) const override {
      std::copy(data_.begin(), data_.end(),
                std::ostreambuf_iterator<char>(stream));
    }

   private:
    uint32_t chunkId_;
    std::vector<char> data_;
  };

  /**
   * @brief FormatInfoChunk的ExtraData的类表示
   */
  class ExtraData {
   public:
    /// @brief ExtraData构建器
    ExtraData(uint16_t validBitsPerSample, uint32_t dwChannelMask,
              uint16_t subFormat, std::string subFormatString)
        : validBitsPerSample_(validBitsPerSample),
          dwChannelMask_(dwChannelMask),
          subFormat_(subFormat),
          subFormatString_(subFormatString) {}

    /// @brief ValidBitsPerSample 每样本有效比特信息获取
    uint16_t validBitsPerSample() const { return validBitsPerSample_; }
    /// @brief 获取DwChannelMask
    uint32_t dwChannelMask() const { return dwChannelMask_; }
    /// @brief 获取SubFormat
    uint16_t subFormat() const { return subFormat_; }
    /// @brief 获取SubFormatString
    std::string subFormatString() const { return subFormatString_; }

   private:
    uint16_t validBitsPerSample_;
    uint32_t dwChannelMask_;
    uint16_t subFormat_;
    std::string subFormatString_;
  };

  /**
   * @brief FormatInfoChunk的类表示
   */
  class FormatInfoChunk : public Chunk {
   public:
    /**
     * @brief FormatInfoChunk简单构建器
     *
     * @param 获取通道数参数
     * @param 获取音频数据的采样率
     * @param 获取文件的位深
     * @param 获取定制ExtraData (如非定制化，则返回nullptr)
     */
    FormatInfoChunk(uint16_t channels, uint32_t sampleRate, uint32_t bitDepth,
                    std::shared_ptr<ExtraData> extraData = nullptr) {
      formatTag_ = 1;
      channelCount_ = channels;
      sampleRate_ = sampleRate;
      bitsPerSample_ = bitDepth;
      extraData_ = extraData;

      // 验证生效
      if (channelCount_ < 1) {
        throw std::runtime_error("channelCount < 1");
      }
      if (sampleRate_ < 1) {
        throw std::runtime_error("sampleRate < 1");
      }
      if (bitsPerSample_ != 16u && bitsPerSample_ != 24u &&
          bitsPerSample_ != 32u) {
        std::stringstream errorString;
        errorString << "bitDepth not supported: " << bitsPerSample_;
        throw std::runtime_error(errorString.str());
      }
    }

    uint32_t id() const override { return utils::fourCC("fmt "); }
    uint64_t size() const override { return 16u; }

    /// @brief 获取FormatTag
    uint16_t formatTag() const { return formatTag_; }
    /// @brief 获取ChannelCount
    uint16_t channelCount() const { return channelCount_; }
    /// @brief 获取SampleRate 
    uint32_t sampleRate() const { return sampleRate_; }
    /// @brief 获取BytesPerSecond
    uint32_t bytesPerSecond() const { return sampleRate() * blockAlignment(); }
    /// @brief 获取BlockAlignment
    uint16_t blockAlignment() const {
      return channelCount() * bitsPerSample() / 8;
    }
    /// @brief 获取BitsPerSample 
    uint16_t bitsPerSample() const { return bitsPerSample_; }

    /// @brief 获取ExtraData
    const std::shared_ptr<ExtraData> extraData() const { return extraData_; }

    void write(std::ostream& stream) const override {
      utils::writeValue(stream, formatTag());
      utils::writeValue(stream, channelCount());
      utils::writeValue(stream, sampleRate());
      utils::writeValue(stream, bytesPerSecond());
      utils::writeValue(stream, blockAlignment());
      utils::writeValue(stream, bitsPerSample());
      if (extraData()) {
        utils::writeValue(stream, extraData()->validBitsPerSample());
        utils::writeValue(stream, extraData()->dwChannelMask());
        utils::writeValue(stream, extraData()->subFormat());
        utils::writeValue(stream, extraData()->subFormatString());
      }
    }

   private:
    uint16_t formatTag_;
    uint16_t channelCount_;
    uint32_t sampleRate_;
    uint16_t bitsPerSample_;
    std::shared_ptr<ExtraData> extraData_;
  };

  /**
   * @brief DataChunk的类
   */
  class DataChunk : public Chunk {
   public:
    DataChunk() { size_ = 0; }

    uint32_t id() const override { return utils::fourCC("data"); }
    uint64_t size() const override { return size_; }

    void setSize(uint64_t size) { size_ = size; }

    /**
     * @brief 非用于将Chunk写入流（stream）Not to be used write chunk to stream
     *
     * @warning 由于data chunk通常不是用一块写的，因此不使用此函数的override。使用此方法将引发异常。
     */
    void write(std::ostream& /* stream */) const override {
      throw std::logic_error(
          "DataChunk::write method is not implemented. Use Bw64Writer::write "
          "instead.");
    }

   private:
    uint64_t size_;
  };

  /**
   * @brief DataChunk的类
   */
  class JunkChunk : public Chunk {
   public:
    JunkChunk() { data_.resize(28, '\0'); }

    uint32_t id() const override { return utils::fourCC("JUNK"); }
    uint64_t size() const override { return data_.size(); }

    void write(std::ostream& stream) const override {
      std::copy(data_.begin(), data_.end(),
                std::ostreambuf_iterator<char>(stream));
    }

   private:
    std::vector<char> data_;
  };

  /**
   * @brief AxmlChunk的类
   */
  class AxmlChunk : public Chunk {
   public:
    static uint32_t Id() { return utils::fourCC("axml"); }

    AxmlChunk(const std::string& axml) {
      std::copy(axml.begin(), axml.end(), std::back_inserter(data_));
    }

    uint32_t id() const override { return AxmlChunk::Id(); }
    uint64_t size() const override { return data_.size(); }

    /*
     * @brief 将AxmlChunk写入流（stream）
     */
    void write(std::ostream& stream) const override {
      std::copy(data_.begin(), data_.end(),
                std::ostreambuf_iterator<char>(stream));
    }

   private:
    std::vector<char> data_;
  };

  /**
   * @brief AudioId域的类
   */
  class AudioId {
   public:
    AudioId(uint16_t trackIndex, const std::string& uid,
            const std::string trackRef, const std::string& packRef) {
      if (uid.size() > 12) {
        std::stringstream errorString;
        errorString << "uid \'" << uid << "\' is too long (" << uid.size()
                    << " > " << 12;
        throw std::runtime_error("uid is too long ");
      }
      if (trackRef.size() > 14) {
        std::stringstream errorString;
        errorString << "trackRef \'" << trackRef << "\' is too long ("
                    << trackRef.size() << " > " << 14;
        throw std::runtime_error("uid is too long ");
      }
      if (packRef.size() > 11) {
        std::stringstream errorString;
        errorString << "packRef \'" << packRef << "\' is too long ("
                    << packRef.size() << " > " << 11;
        throw std::runtime_error("packRef is too long ");
      }

      // 用whitspace初始化数组
      std::memset(uid_, ' ', 12);
      std::memset(trackRef_, ' ', 14);
      std::memset(packRef_, ' ', 11);

      // 保存值
      trackIndex_ = trackIndex;
      std::copy(uid.begin(), uid.end(), uid_);
      std::copy(trackRef.begin(), trackRef.end(), trackRef_);
      std::copy(packRef.begin(), packRef.end(), packRef_);
    }

    /*
     * @brief 获取TrackIndex 
     *
     * @returns 返回AudioId的音轨索引
     */
    uint16_t trackIndex() const { return trackIndex_; };
    /*
     * @brief 获取audioTrackUID 
     *
     * @returns 返回AudioId的audioTrackUID
     */
    std::string uid() const { return std::string(uid_, 12); }
    /*
     * @brief 获取audioTrackFormatID 
     *
     * @returns 返回AudioId的audioTrackFormatID
     */
    std::string trackRef() const { return std::string(trackRef_, 14); }
    /*
     * @brief 获取audioPackFormatID 
     *
     * @returns 返回AudioId的audioPackFormatID
     */
    std::string packRef() const { return std::string(packRef_, 11); }

    /*
     * @brief 将AudioId写入流
     */
    void write(std::ostream& stream) const {
      utils::writeValue(stream, trackIndex());
      utils::writeValue(stream, uid_);
      utils::writeValue(stream, trackRef_);
      utils::writeValue(stream, packRef_);
      utils::writeValue(stream, ' ');  // 填充
    }

    bool operator==(const AudioId& rhs) const {
      return trackIndex() == rhs.trackIndex() && uid() == rhs.uid() &&
             trackRef() == rhs.trackRef() && packRef() == rhs.packRef();
    }
    bool operator!=(const AudioId& rhs) const { return !(*this == rhs); }

   private:
    uint16_t trackIndex_;
    char uid_[12];
    char trackRef_[14];
    char packRef_[11];
  };

  /**
   * @brief ChnaChunk的类
   */
  class ChnaChunk : public Chunk {
   public:
    ChnaChunk(){};
    ChnaChunk(std::initializer_list<AudioId> audioIds) : audioIds_(audioIds){};
    ChnaChunk(std::vector<AudioId> audioIds) : audioIds_(audioIds){};

    uint32_t id() const override { return utils::fourCC("chna"); }
    uint64_t size() const override {
      return sizeof(numTracks()) + sizeof(numUids()) + numUids() * 40;
    }

    /// @brief 音轨数 
    uint16_t numTracks() const {
      std::set<uint16_t> trackIndices;
      for (auto audioId : audioIds()) {
        trackIndices.insert(audioId.trackIndex());
      }
      return static_cast<uint16_t>(trackIndices.size());
    }
    /// @brief 获取NumUids 
    uint16_t numUids() const { return static_cast<uint16_t>(audioIds_.size()); }
    /// @brief 获取AudioIds 
    std::vector<AudioId> audioIds() const { return audioIds_; }
    /// @brief 将AudioId加入到AudioId表
    void addAudioId(AudioId id) { audioIds_.push_back(id); }

    void write(std::ostream& stream) const override {
      utils::writeValue(stream, numTracks());
      utils::writeValue(stream, numUids());
      for (auto audioId : audioIds()) {
        audioId.write(stream);
      }
    }

   private:
    std::vector<AudioId> audioIds_;
  };

  /**
   * @brief DataSize64 chunk的类
   */
  class DataSize64Chunk : public Chunk {
   public:
    /// @brief DataSize64Chunk构建器
    DataSize64Chunk(
        uint64_t bw64Size = 0, uint64_t dataSize = 0,
        std::map<uint32_t, uint64_t> table = std::map<uint32_t, uint64_t>())
        : bw64Size_(bw64Size), dataSize_(dataSize), table_(table) {
      dummySize_ = 0;
    }

    uint32_t id() const override { return utils::fourCC("ds64"); }
    uint64_t size() const override {
      return sizeof(bw64Size()) + sizeof(dataSize()) + sizeof(dummySize()) +
             sizeof(tableLength()) + table_.size() * 12;
    }
    /// @brief 获取Bw64Size 
    uint64_t bw64Size() const { return bw64Size_; }
    /// @brief 获取DataSize 
    uint64_t dataSize() const { return dataSize_; }
    /// @brief 获取DummySize 
    uint64_t dummySize() const { return dummySize_; }
    /// @brief 获取TableLength 
    uint32_t tableLength() const {
      return static_cast<uint32_t>(table_.size());
    }

    /// @brief 设置Bw64Size 
    void bw64Size(uint64_t size) { bw64Size_ = size; }
    /// @brief 设置DataSize 
    void dataSize(uint64_t size) { dataSize_ = size; }
    /// @brief 设置DummySize
    void dummySize(uint64_t size) { dummySize_ = size; }

    /// @brief 获取列表
    const std::map<uint32_t, uint64_t>& table() const { return table_; }

    /// @brief 确定id的chunkSize
    bool hasChunkSize(uint32_t id) const { return table_.count(id) != 0; }
    /// @brief 获取id的chunkSize
    uint64_t getChunkSize(uint32_t id) const { return table_.at(id); }
    /// @brief 设置或增添ChunkSize
    void setChunkSize(uint32_t id, uint64_t size) {
      if (id == utils::fourCC("bw64")) {  //
        bw64Size_ = size;
      } else if (id == utils::fourCC("data")) {
        dataSize_ = size;
      } else {
        table_[id] = size;
      }
    }

    /// @brief 从列表中删除ChunkSize
    void removeChunkSize(uint32_t id) { table_.erase(id); }
    /// @brief 清理ChunkSize列表
    void clearChunkSizeTable() { table_.clear(); }

    void write(std::ostream& stream) const override {
      utils::writeValue(stream, bw64Size());
      utils::writeValue(stream, dataSize());
      utils::writeValue(stream, dummySize());
      utils::writeValue(stream, tableLength());
      for (auto& entry : table()) {
        utils::writeValue(stream, entry.first);  // chunkId
        utils::writeValue(stream, entry.second);  // chunkSize
      }
    }

   private:
    uint64_t bw64Size_;
    uint64_t dataSize_;
    uint64_t dummySize_;
    std::map<uint32_t, uint64_t> table_;
  };

}  // namespace bw64
