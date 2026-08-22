#pragma once
#include <boost/variant.hpp>
#include "common_types.hpp"
#include "export.hpp"

namespace render {

  struct RENDER_EXPORT CartesianScreen {
    double aspectRatio;
    CartesianPosition centrePosition;
    double widthX;
  };

  struct RENDER_EXPORT PolarScreen {
    double aspectRatio;
    PolarPosition centrePosition;
    double widthAzimuth;
  };

  using Screen = boost::variant<PolarScreen, CartesianScreen>;

  Screen RENDER_EXPORT getDefaultScreen();

}  // namespace render
