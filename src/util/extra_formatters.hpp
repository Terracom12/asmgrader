#pragma once

#include <fmt/format.h>

#include <optional>
#include <string>

namespace std {
/// Formatter for std::optional<T>
template <typename T>
inline std::string format_as(const std::optional<T>& from) {
    if (!from) {
        return "nullopt";
    }

    return fmt::format("Optional({})", from.value());
}

} // namespace std
