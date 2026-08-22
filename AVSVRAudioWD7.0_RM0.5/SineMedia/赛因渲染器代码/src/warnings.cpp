#include "render/warnings.hpp"
#include <iostream>

namespace render {
  void default_warning_cb_fn(const render::Warning &warning) {
    std::cerr << "librender: warning: " << warning.message << std::endl;
  }

  const WarningCB default_warning_cb = default_warning_cb_fn;
}  // namespace render
