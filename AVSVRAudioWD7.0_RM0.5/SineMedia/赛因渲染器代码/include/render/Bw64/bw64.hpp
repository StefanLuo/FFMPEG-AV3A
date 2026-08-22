/**
  * libbw64主头文件，是用户代码需要调用的唯一文件。
 */
#pragma once
#include "reader.hpp"
#include "writer.hpp"

namespace bw64 {

  /**
   * @功能 打开BW64文件进行读取
   *
   * @参数 要读取的文件的文件名路径参数
   *
   * 打开BW64文件进行读取的函数。
   *
   * @返回值：Bw64Reader实例，可用于读取样本
   */
  inline std::unique_ptr<Bw64Reader> readFile(const std::string& filename) {
    return std::unique_ptr<Bw64Reader>(new Bw64Reader(filename.c_str()));
  }

  /**
   * @功能 打开BW64文件进行写入
   *
   * 打开一个新的BW64文件进行写入的函数，可用于添加'axml'和'chna'块。
   *
   * 传递给此函数，如果在写入文件之前已经知道所有组件，
   * 则“axml”和“chna”块将添加到BW64文件实际数据块的前面。
   *
   * @参数 filename 要写入的文件的路径
   * @参数 channels 新文件的通道计数
   * @参数 sampleRate 新文件的采样率
   * @参数 bitDepth 新文件的目标位深
   * @参数 chnaChunk 条件适用下，将被调用的通道分配chuank
   * @参数 axmlChunk 条件适用下将被调用的AXML chunk
   *
   * @返回值：Bw64Writer实例，可用于写入样本
   *
   */
  inline std::unique_ptr<Bw64Writer> writeFile(
      const std::string& filename, uint16_t channels = 1u,
      uint32_t sampleRate = 48000u, uint16_t bitDepth = 24u,
      std::shared_ptr<ChnaChunk> chnaChunk = nullptr,
      std::shared_ptr<AxmlChunk> axmlChunk = nullptr) {
    std::vector<std::shared_ptr<Chunk>> additionalChunks;
    if (chnaChunk) {
      additionalChunks.push_back(chnaChunk);
    }
    if (axmlChunk) {
      additionalChunks.push_back(axmlChunk);
    }
    return std::unique_ptr<Bw64Writer>(new Bw64Writer(
        filename.c_str(), channels, sampleRate, bitDepth, additionalChunks));
  }

}  // namespace bw64
