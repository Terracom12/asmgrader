#pragma once

namespace asmgrader::common {

/// Usefule as the default argument to function callbacks
constexpr auto noop = [](auto&&... /*discarded*/) {};

} // namespace asmgrader::common
