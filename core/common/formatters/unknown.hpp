#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

#include <string>

namespace asmgrader {

template <typename T>
inline std::string fmt_or_unknown(T&& value, fmt::fstring<T> fmt = "{}") {
    if constexpr (fmt::formattable<T>) {
        return fmt::format(fmt, std::forward<T>(value));
    } else {
        return "<unknown>";
    }
}

} // namespace asmgrader
