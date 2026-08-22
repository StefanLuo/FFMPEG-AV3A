#pragma once

#include <complex>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "../export.hpp"
#include "../fft.hpp"

namespace render {
  namespace dsp {
    namespace block_convolver_impl {
      class Context;
      class Filter;
      class BlockConvolver;
    }  // 将block_convolver_impl定义为namespace

    namespace block_convolver {
      /// 实际数据类型（浮点）。
      using real_t = float;
      /// 复杂数据的类型。
      using complex_t = std::complex<real_t>;

      /** 执行特定块大小的卷积所需的静态数据；可以在任意数量的块卷积器和过滤器实例之间共享。
      */
      class RENDER_EXPORT Context {
       public:
        /** 创建具有给定块大小的上下文。
         * @param 获取样本中数据块的大小值。
         * @param FFT实现的使用参数。
         */
        Context(size_t block_size, FFTImpl<real_t> &fft_impl);

       private:
        std::shared_ptr<block_convolver_impl::Context> impl;
        friend class Filter;
        friend class BlockConvolver;
      };

      /** 可在多个区块卷积器BlockConvolver实例之间共享的滤波器响应。
       *
       * 这里将存储预转换的滤波器数据块。 */
      class RENDER_EXPORT Filter {
       public:
        Filter(const Context &ctx, size_t n, const real_t *filter);

        /** 滤波器中的数据块数。The number of blocks in the filter. */
        size_t num_blocks() const;

       private:
        std::shared_ptr<block_convolver_impl::Filter> impl;
        friend class BlockConvolver;
      };

      /** BlockConvolver 以固定的数据块大小和滤波器之间有效衰减，来实现分区重叠加卷积。
       */
      class RENDER_EXPORT BlockConvolver {
       public:
        /** 根据数据块大小和块数创建块卷积器BlockConvolver。
         * @param ctx 转换所需的相关参数。
         * @param num_blocks 使用的任何筛选器的最大数据块数。
         */
        BlockConvolver(const Context &ctx, size_t num_blocks);

        /** Create a BlockConvolver given the block size and number of blocks.
         *  If filter == nullptr, num_blocks must be specified.
         * @param ctx 转换所需的相关参数。
         * @param filter 需使用的初始筛选器，或将无筛选器的滤波器进行nullptr代管。
         * @param num_blocks 将使用的任何滤波器的数据块数最大化；使用0将从通过的滤波器中获取数据块数。
         */
        BlockConvolver(const Context &ctx, const Filter &filter,
                       size_t num_blocks = 0);

        ~BlockConvolver();

        /** 将一块音频通过滤波器。
         * @param in 输入block_size长度的样本
         * @param out  输出block_size长度的样本
         */
        void process(const float *in, float *out);

        /** 在下一个块中交叉淡入新的过滤器。
         *
         * 等同于：
         * - 创建一个新的卷积。
         * - 通过新旧卷积器传递下一个样本块，其中对旧样本块的输入在该块上淡出，
         * 对新样本块的输入在该块上淡入。所有后续块都将通过新过滤器。
         * - 为下一个num_blocks新旧滤波器建立混合输出。
         */
        void crossfade_filter(const Filter &filter);

        /** 零值滤波器的交叉衰减 */
        void fade_down();

        /** 在下一个块的开头切换到其他滤波器。
         */
        void set_filter(const Filter &filter);

        /** 在下一个块的开头切换到零值滤波器。 */
        void unset_filter();

       private:
        std::unique_ptr<block_convolver_impl::BlockConvolver> impl;
      };
    }  // namespace block_convolver
  }  // namespace dsp
}  // namespace render
