#pragma once
#include "export.hpp"
#include "layout.hpp"

namespace render{

  /// Get all ITU-R BS.2051 layouts.
  RENDER_EXPORT std::vector<Layout> loadLayouts();

  /// Get a layout given its ITU-R BS.2051 name (e.g. `4+5+0`).
  RENDER_EXPORT Layout getLayout(const std::string& name);

}  // namespace render
