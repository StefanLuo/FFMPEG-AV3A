#pragma once
#include "../exceptions.hpp"

namespace render {
  // implementation for _assert_impl. This is wrapped in a macro so that we can
  // use __LINE__, __FILE__ etc. in the future.
  inline void _assert_impl(bool condition, const std::string &message) {
    if (!condition) throw internal_error(message);
  }
}  // namespace render

// assert which is always enabled, and results in an render::internal_error with
// the given message
#define render_assert(condition, message) render::_assert_impl((condition), (message))
