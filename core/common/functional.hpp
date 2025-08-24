#pragma once

namespace common {

/// Usefule as the default argument to function callbacks
constexpr auto noop = [](auto&&... /*discarded*/) {};

} // namespace common
