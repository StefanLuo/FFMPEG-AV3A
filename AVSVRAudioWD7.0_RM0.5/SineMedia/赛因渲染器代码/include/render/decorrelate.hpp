#pragma once
#include <vector>
#include "export.hpp"
#include "layout.hpp"

namespace render {
  /** @brief 为扬声器布局中的每个通道设计一个滤波器。Design one filter for each channel in layout.
   *
   * @param layout 为要设计的扬声器布局设置参数；通道名称用来为通道分配滤波器。
   *
   * @return 返回值：去相关滤波器
   */
  template <typename T = float>
  RENDER_EXPORT std::vector<std::vector<T>> designDecorrelators(Layout layout);

  /** @brief 获取补偿去相关器所需的延迟长度
   *
   * @return 返回值：样本中的延迟长度。
   */
  RENDER_EXPORT int decorrelatorCompensationDelay();
}  // namespace render
