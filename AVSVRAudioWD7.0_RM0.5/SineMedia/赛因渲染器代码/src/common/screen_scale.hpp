#pragma once
#include <boost/optional.hpp>
#include "render/screen.hpp"

namespace render {
  class ScreenScaleHandler {
   public:
    ScreenScaleHandler(boost::optional<Screen> reproductionScreen)
        : _reproductionScreen(reproductionScreen){};

   private:
    boost::optional<Screen> _reproductionScreen;
  };
}  // namespace render
