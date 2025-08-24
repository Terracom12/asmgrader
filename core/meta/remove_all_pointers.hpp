#pragma once

#include <concepts>

namespace asmgrader {

// NOLINTBEGIN(readability-identifier-naming)

/// Removes all levels of pointers on a type
template <typename T>
struct remove_all_pointers
{
    using type = T;
};

/// Removes all levels of pointers on a type
template <typename T>
struct remove_all_pointers<T*>
{
    using type = remove_all_pointers<T>::type;
};

template <typename T>
using remove_all_pointers_t = remove_all_pointers<T>::type;

// NOLINTEND(readability-identifier-naming)

static_assert(std::same_as<remove_all_pointers_t<int>, int>);
static_assert(std::same_as<remove_all_pointers_t<int*>, int>);
static_assert(std::same_as<remove_all_pointers_t<int**>, int>);
static_assert(std::same_as<remove_all_pointers_t<int***>, int>);
// NOLINTBEGIN(*c-arrays)
static_assert(std::same_as<remove_all_pointers_t<int[]>, int[]>);
static_assert(std::same_as<remove_all_pointers_t<int (*)[]>, int[]>);
static_assert(std::same_as<remove_all_pointers_t<int (**)[]>, int[]>);
static_assert(std::same_as<remove_all_pointers_t<int*[]>, int*[]>);
static_assert(std::same_as<remove_all_pointers_t<int*[]>, int*[]>);
// NOLINTEND(*c-arrays)

} // namespace asmgrader
