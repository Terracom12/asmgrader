#pragma once

#include "common/formatters/debug.hpp"

#include <fmt/base.h>

namespace util::detail {

template <typename T, typename Enable = void>
struct FormatterImpl;

} // namespace util::detail

/// Formatter specialization for anything supported by FormatterImpl
template <typename T>
    requires requires { util::detail::FormatterImpl<T>(); }
struct fmt::formatter<T> : DebugFormatter
{
    constexpr auto format(const T& from, fmt::format_context& ctx) const {
        return util::detail::FormatterImpl<T>::format(from, ctx);
    }
};
