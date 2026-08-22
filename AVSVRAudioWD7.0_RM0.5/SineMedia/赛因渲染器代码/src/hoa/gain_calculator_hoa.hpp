#pragma once
#include <Eigen/Core>
#include <memory>
#include "render/helpers/output_gains.hpp"
#include "render/layout.hpp"
#include "render/metadata.hpp"
#include "render/warnings.hpp"

namespace render {

  class GainCalculatorHOAImpl {
   public:
    GainCalculatorHOAImpl(const Layout& layout);

    void calculate(const HOATypeMetadata& metadata, OutputGainMat& direct,
                   const WarningCB& warning_cb = default_warning_cb);

    template <typename T>
    void calculate(const HOATypeMetadata& metadata,
                   std::vector<std::vector<T>>& direct,
                   const WarningCB& warning_cb = default_warning_cb) {
      OutputGainMatVecT<T> direct_wrap(direct);
      calculate(metadata, direct_wrap, warning_cb);
    }

   private:
    Eigen::Matrix<double, Eigen::Dynamic, 3> points;
    Eigen::Array<bool, Eigen::Dynamic, 1> is_lfe;
    Eigen::MatrixXd G_virt;
  };

}  // namespace render
