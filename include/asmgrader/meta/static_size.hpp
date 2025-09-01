#pragma once

#include <range/v3/range/concepts.hpp>

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace asmgrader {

template <typename Range>
concept HasStaticSize = requires {
    // have to use ::value here to force instantiation of tuple_size
    { std::tuple_size<std::decay_t<Range>>::value } -> std::convertible_to<std::size_t>;
};

template <ranges::range Range>
consteval std::size_t get_static_size() {
    return std::tuple_size_v<std::decay_t<Range>>;
}

template <ranges::range Range>
constexpr std::size_t get_static_size_or(std::size_t default_value) {
    if constexpr (HasStaticSize<Range>) {
        return get_static_size<Range>();
    }

    return default_value;
}

} // namespace asmgrader
