#pragma once

#include <type_traits>
namespace util {

/// Very simple type for better semantics when we want
/// static_assert(false) to work inside primary class templates

template <typename T>
struct always_false : std::false_type
{
};

template <typename T>
constexpr bool always_false_v = always_false<T>::value;

} // namespace util
