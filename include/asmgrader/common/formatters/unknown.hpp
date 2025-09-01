#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/iterator/concepts.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace asmgrader {

template <typename T>
    requires(fmt::formattable<T>)
inline std::string format_or_unknown(T&& value, fmt::format_string<T> fmt = "{}") {
    return fmt::format(fmt::runtime(fmt.str), std::forward<T>(value));
}

template <typename T>
    requires(!fmt::formattable<T>)
inline std::string format_or_unknown(T&& /*value*/, fmt::format_string<std::string_view> fmt = "{}") {

    return fmt::format(fmt::runtime(fmt.str), "<unknown>");
}

// fmt's iterators do not satisfy standard requirements
template </*ranges::output_iterator<char>*/ typename OutputIt, typename T>
    requires(fmt::formattable<T>)
inline auto format_to_or_unknown(OutputIt&& out, T&& value, fmt::format_string<T> fmt = "{}") {
    return fmt::format_to(std::forward<OutputIt>(out), fmt::runtime(fmt), std::forward<T>(value));
}

template </*ranges::output_iterator<char>*/ typename OutputIt, typename T>
    requires(!fmt::formattable<T>)
inline auto format_to_or_unknown(OutputIt&& out, T&& /*value*/, fmt::format_string<std::string_view> fmt = "{}") {
    return fmt::format_to(std::forward<OutputIt>(out), fmt::runtime(fmt), "<unknown>");
}

} // namespace asmgrader
