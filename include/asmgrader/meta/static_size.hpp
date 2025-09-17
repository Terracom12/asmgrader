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

template <typename Range>
consteval std::size_t get_static_size() {
    using Type = std::remove_cvref_t<Range>;
    if constexpr (std::is_array_v<Type>) {
        return std::extent_v<Type, 0>;
    } else {
        return std::tuple_size_v<Type>;
    }
}

template <typename Range>
constexpr std::size_t get_static_size_or(std::size_t default_value) {
    if constexpr (HasStaticSize<Range>) {
        return get_static_size<Range>();
    }

    return default_value;
}

} // namespace asmgrader
