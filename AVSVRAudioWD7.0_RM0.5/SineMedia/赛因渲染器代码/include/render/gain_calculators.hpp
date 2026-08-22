#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "export.hpp"
#include "layout.hpp"
#include "metadata.hpp"
#include "warnings.hpp"

namespace render {

  class GainCalculatorDirectSpeakersImpl;
  class GainCalculatorObjectsImpl;
  class GainCalculatorHOAImpl;

  /// typeDefinition==“DirectSpeakers”的增益计算器
  class RENDER_EXPORT GainCalculatorDirectSpeakers {
   public:
    GainCalculatorDirectSpeakers(
        const Layout& layout,
        std::map<std::string, std::string> additionalSubstitutions = {});
    ~GainCalculatorDirectSpeakers();

    /// 计算元数据的增益。\p gains包含用于渲染此频道的每个扬声器增益。
    
    template <typename T>
    void calculate(const DirectSpeakersTypeMetadata& metadata,
                   std::vector<T>& gains,
                   const WarningCB& warning_cb = default_warning_cb);

   private:
    std::unique_ptr<GainCalculatorDirectSpeakersImpl> _impl;
  };

  /// typeDefinition == "Objects"的增益计算器
  class RENDER_EXPORT GainCalculatorObjects {
   public:
    GainCalculatorObjects(const Layout& layout);
    ~GainCalculatorObjects();

    /// 计算元数据的增益。\p directGains和\p diffuseGains包含用于渲染此通道的每个扬声器增益。
    ///
    /// 使用这些增益时：
    /// - \p directGains应用该通道时，与其他对象相加，形成n通道的direct bus
    
    /// - \p diffuseGains应用于该通道，与其他对象相加，形成n通道漫diffuse bus
    
    /// - 使用designDecorrelators（）给出的相应FIR滤波器，处理diffuse bus中的每个通道
    
    /// - 直接总线中的每个通道都由decorrelatorCompensationDelay（）采样来延迟，
    ///   再通过去相关滤波器补偿延迟
    
    /// - 解相关滤波器的输出和延迟混合在一起形成输出
    
    template <typename T>
    void calculate(const ObjectsTypeMetadata& metadata,
                   std::vector<T>& directGains, std::vector<T>& diffuseGains,
                   const WarningCB& warning_cb = default_warning_cb);

   private:
    std::unique_ptr<GainCalculatorObjectsImpl> _impl;
  };

  /// typeDefinition == "HOA"的增益计算器
  class RENDER_EXPORT GainCalculatorHOA {
   public:
    GainCalculatorHOA(const Layout& layout);
    ~GainCalculatorHOA();

    /// 计算元数据的解码矩阵。
    /// 增益包含每个输入通道每个扬声器增益的一个向量。
    
    template <typename T>
    void calculate(const HOATypeMetadata& metadata,
                   std::vector<std::vector<T>>& gains,
                   const WarningCB& warning_cb = default_warning_cb);

   private:
    std::unique_ptr<GainCalculatorHOAImpl> _impl;
  };

}  // namespace render
