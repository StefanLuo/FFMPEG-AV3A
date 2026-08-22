#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <iostream>

#include "../helpers/assert.hpp"

namespace render {
  namespace dsp {
    /// 用于将插入增益向量应用于音频样本的实用程序; 
    /// 有关详细信息，请参阅adm_render:：include:：render:：dsp:：GainInterpolator。

    /// 用于索引到样本缓冲区的类型。
    
    /// 增益插值器，在插值类型上模板化，该类型定义插值类型（线性、余弦等）、
    /// 要在之间插值的值类型（浮点、向量、矩阵），从而限制输入和输出通道大小。
    ///
    /// 插值曲线由#interp_points中的点定义。每一个都是一对当时的样本索引和增益值。
    /// 这些必须按时间顺序排序。重复时间可用于指定步骤。
    ///
    /// 有关可能的插值类型，请参见LinearInterpSingle、LinearInterpVectorSee LinearInterpSingle, LinearInterpVector and LinearInterpMatrix for
    /// 和LinearInterpMatrix。有关插值类型定义的接口，请参见InterpType。
    //
    // 在内部，两个插值点之间的采样块由“块索引”引用，它没有末端。
    // 该索引是interp_points中块末端的索引，但interp_points.size（）除外，
    // 具有3个插值点的示例：
    //
    // interp_points:   0   1   2
    // block_idx:     0 | 1 | 2 | 3

    using SampleIndex = long int;

    template <typename InterpType>
    class GainInterpolator {
     public:
      std::vector<std::pair<SampleIndex, typename InterpType::Point>>
          interp_points;

      /// 处理n个样本。
      /// \param block_start 相对于#interp_points中的样本索引的第一个样本索引参数
      /// \param nsamples 在\p in and \p out中的样本数
      /// \param in 具有与插值类型和所用点兼容的多个通道的输入样本参数
      /// \param out 使用与插值类型和所用点兼容的多个通道输出采样
      void process(SampleIndex block_start, size_t nsamples,
                   const float *const *in, float *const *out) {
        SampleIndex block_end = block_start + nsamples;
        SampleIndex this_block_start = block_start;

        while (this_block_start < block_end) {
          size_t block_idx = find_block(this_block_start);

          SampleIndex this_block_end =
              block_idx == interp_points.size()
                  ? block_end
                  : std::min(interp_points[block_idx].first, block_end);
          render_assert(this_block_start < this_block_end,
                     "found block ends before processed block starts");

          if (block_idx == 0 || block_idx == interp_points.size() ||
              InterpType::constant_interp(interp_points[block_idx - 1].second,
                                          interp_points[block_idx].second)) {
            size_t point_with_value =
                block_idx == interp_points.size() ? block_idx - 1 : block_idx;
            InterpType::apply_constant(in, out, this_block_start - block_start,
                                       this_block_end - block_start,
                                       interp_points[point_with_value].second);
          } else {
            InterpType::apply_interp(in, out, this_block_start - block_start,
                                     this_block_end - block_start, block_start,
                                     interp_points[block_idx - 1].first,
                                     interp_points[block_idx].first,
                                     interp_points[block_idx - 1].second,
                                     interp_points[block_idx].second);
          }

          this_block_start = this_block_end;
        }
      }

     private:
      // 查找样本是在块之前、之后还是在块内部：
      //  0: 样本在块内
      // -1: 样本位于块开始之前
      //  1: 样本位于块的末尾
      int block_cmp(size_t block_idx, SampleIndex sample_idx) {
        if (block_idx > 0) {
          SampleIndex block_start = interp_points[block_idx - 1].first;
          if (sample_idx < block_start) return -1;
        }

        if (block_idx < interp_points.size()) {
          SampleIndex block_end = interp_points[block_idx].first;
          if (sample_idx >= block_end) return 1;
        }

        return 0;
      }

      // 查找样本的块索引；返回的最后一个块是缓存的，因此不必查看很多块
      size_t last_block = 0;
      size_t find_block(SampleIndex sample_idx) {
        // 如果自上次调用后interp_points发生更改，则last_block可能超出允许范围
        if (last_block > interp_points.size()) last_block = 0;

        // 沿块_cmp给出的方向移动，直到找到正确的块（cmp==0）；
        int cmp = block_cmp(last_block, sample_idx);
        int first_cmp = cmp;
        while (cmp != 0) {
          last_block += cmp;
          if (cmp != first_cmp)
            throw invalid_argument("interpolation points are not sorted");
          cmp = block_cmp(last_block, sample_idx);
        }

        return last_block;
      }
    };

    /// 插值类型的基类型。
    template <typename PointT>
    struct InterpType {
      /// 插值曲线上点的类型，例如浮点、向量、矩阵。
      using Point = PointT;

      /// 判断两个点是否相同（应在它们之间使用常量/无插值）
      static bool constant_interp(const Point &a, const Point &b) {
        return a == b;
      }

      /// 将插值增益应用于\p in，并写入\p out。
      ///
      /// 例如，如果插值曲线在样本5和15之间从x变为y，
      /// 则第一个和第二个10个样本块将发生以下调用：
      ///
      /// \code
      /// apply_interp(in_a, out_a, 5, 10, 0, 5, 15, x, y);
      /// apply_interp(in_b, out_b, 0, 5, 10, 5, 15, x, y);
      /// \endcode
      ///
      /// \param in 输入样本
      /// \param out 输出样本
      /// \param range_start 在\p in和\p out中的偏移量开始应用处理
      /// \param range_end 在\p in和\p out中的偏移量结束应用处理
      /// \param block_start: 启动此块的示例索引，即从[0][0]
      /// \param start: 启动插值曲线的样本索引
      /// \param end: 结束插值曲线的样本索引
      /// \param start_point:在\p 开始设置增益值
      /// \param end_point: 在\p 结束设置增益
      static void apply_interp(const float *const *in, float *const *out,
                               SampleIndex range_start, SampleIndex range_end,
                               SampleIndex block_start, SampleIndex start,
                               SampleIndex end, const Point &start_point,
                               const Point &end_point);

      /// 在\p in获取增益和\p out写入中使用constnt。
      ///
      /// \param in 输入样本
      /// \param out 输出样本
      /// \param range_start 在\p in和\p out中的偏移量开始应用处理
      /// \param range_end 在\p in和\p out中的偏移量结束应用处理
      /// \param point: 将应用的增益值
      static void apply_constant(const float *const *in, float *const *out,
                                 SampleIndex range_start, SampleIndex range_end,
                                 const Point &point);
    };

    // 插值实现

    /// GainInterpolator中使用的单通道线性插值。
    struct LinearInterpSingle : public InterpType<float> {
      static void apply_interp(const float *const *in, float *const *out,
                               SampleIndex range_start, SampleIndex range_end,
                               SampleIndex block_start, SampleIndex start,
                               SampleIndex end, const Point &start_point,
                               const Point &end_point) {
        float scale = 1.0f / (end - start);
        for (SampleIndex i = range_start; i < range_end; i++) {
          float p = (float)((block_start + i) - start) * scale;

          float gain = (1.0f - p) * start_point + p * end_point;

          out[0][i] = in[0][i] * gain;
        }
      }

      static void apply_constant(const float *const *in, float *const *out,
                                 SampleIndex range_start, SampleIndex range_end,
                                 const Point &point) {
        for (SampleIndex i = range_start; i < range_end; i++) {
          out[0][i] = in[0][i] * point;
        }
      }
    };

    /// 线性插值，一个通道输入，多个通道输出，用于GainInterpolator。
    struct LinearInterpVector : public InterpType<std::vector<float>> {
      static void apply_interp(const float *const *in, float *const *out,
                               SampleIndex range_start, SampleIndex range_end,
                               SampleIndex block_start, SampleIndex start,
                               SampleIndex end, const Point &start_point,
                               const Point &end_point) {
        float scale = 1.0f / (end - start);
        for (SampleIndex i = range_start; i < range_end; i++) {
          float p = (float)((block_start + i) - start) * scale;

          for (size_t channel = 0; channel < start_point.size(); channel++) {
            float gain =
                (1.0f - p) * start_point[channel] + p * end_point[channel];

            out[channel][i] = in[0][i] * gain;
          }
        }
      }

      static void apply_constant(const float *const *in, float *const *out,
                                 SampleIndex range_start, SampleIndex range_end,
                                 const Point &point) {
        for (SampleIndex i = range_start; i < range_end; i++) {
          for (size_t channel = 0; channel < point.size(); channel++) {
            out[channel][i] = in[0][i] * point[channel];
          }
        }
      }
    };

    /// 具有多个输入和输出通道的线性插值，具有用于GainInterpolator的系数矩阵。
    ///
    /// 每个输入通道包含一个输出增益矢量的点。
    struct LinearInterpMatrix : public InterpType<std::vector<std::vector<float>>> 
	{
      static void apply_interp(const float *const *in, float *const *out,
                               SampleIndex range_start, SampleIndex range_end,
                               SampleIndex block_start, SampleIndex start,
                               SampleIndex end, const Point &start_point,
                               const Point &end_point) 
	  {
        float scale = 1.0f / (end - start);
        for (SampleIndex i = range_start; i < range_end; i++) 
		{
          for (size_t out_channel = 0; out_channel < (start_point.size() ? start_point[0].size() : 0); out_channel++) 
		  {
            out[out_channel][i] = 0.0;
          }

          float p = (float)((block_start + i) - start) * scale;
        //  std::cout << "1apply_interp:" << p <<std::endl;
          for (size_t in_channel = 0; in_channel < start_point.size();in_channel++) 
		  {
            for (size_t out_channel = 0; out_channel < start_point[in_channel].size(); out_channel++)
			{
              float gain = (1.0f - p) * start_point[in_channel][out_channel] + p * end_point[in_channel][out_channel];
             // std::cout << "apply_interpgain:" << gain << std::endl;
             /* if (gain > 0 && gain < 1)
              {
                  gain = 1;
              }*/
              out[out_channel][i] += in[in_channel][i] * gain;
            }
          }
        }
      }

      static void apply_constant(const float *const *in, float *const *out,
                                 SampleIndex range_start, SampleIndex range_end,
                                 const Point &point) {
        for (SampleIndex i = range_start; i < range_end; i++) {
          for (size_t out_channel = 0;
               out_channel < (point.size() ? point[0].size() : 0);
               out_channel++) {
            out[out_channel][i] = 0.0;
          }
          for (size_t in_channel = 0; in_channel < point.size(); in_channel++) {
            for (size_t out_channel = 0; out_channel < point[in_channel].size();
                 out_channel++) {
              out[out_channel][i] +=
                  in[in_channel][i] * point[in_channel][out_channel];
            }
          }
        }
      }
    };
  }  // namespace dsp
}  // namespace render
