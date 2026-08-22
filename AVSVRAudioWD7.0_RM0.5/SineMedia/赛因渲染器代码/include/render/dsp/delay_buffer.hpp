#pragma once
#include <memory>
#include "../exceptions.hpp"
#include "../export.hpp"

namespace render {
  namespace dsp {

    class DelayBufferImpl;

    /// 多通道延迟缓存
    class RENDER_EXPORT DelayBuffer {
     public:
      /// @param nchannels 设定输入和输出通道的数量
      /// @param nsamples 设定延迟长度
      DelayBuffer(size_t nchannels, size_t nsamples);

      /// 处理任意数量的样本。 \p input and \p output have
      /// \c nchannels channels and \p nsamples samples.
      void process(size_t nsamples, const float *const *input,
                   float *const *output);

      /// 获得样本中的延迟。
      int get_delay() const;

      ~DelayBuffer();

     private:
      std::unique_ptr<DelayBufferImpl> impl;
    };

  }  // namespace dsp
}  // namespace render
