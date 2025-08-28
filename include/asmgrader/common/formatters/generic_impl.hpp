#pragma once

#include <asmgrader/common/formatters/debug.hpp>

#include <fmt/base.h>

namespace asmgrader::detail {

template <typename T, typename Enable = void>
struct FormatterImpl;

} // namespace asmgrader::detail

/// Formatter specialization for anything supported by FormatterImpl
template <typename T>
    requires requires { ::asmgrader::detail::FormatterImpl<T>(); }
struct fmt::formatter<T> : ::asmgrader::DebugFormatter
{
    constexpr auto format(const T& from, fmt::format_context& ctx) const {
        return ::asmgrader::detail::FormatterImpl<T>::format(from, ctx);
    }
};
