/// \file
/// constexpr capable functions from c-style headers
///
/// All functions in this file will fall back to the c-style implementations when not in a constant-evaluated context,
/// so runtime performance is guaranteed to be *at least* as good. Also fixes UB issues with `cctype` functions.
///
/// Refer to the [cppreference page on std lib headers](https://en.cppreference.com/w/cpp/headers.html) for the pages I
/// used as authoritative definitions in implementing these functions.
#pragma once

#include <asmgrader/common/aliases.hpp>

#include <libassert/assert.hpp>

#include <cctype>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <limits>
#include <type_traits>

namespace asmgrader {

namespace detail {

/// inclusive
constexpr bool in_range(auto&& val, auto&& low, auto&& high) {
    return val >= low && val <= high;
}

template <typename Type>
consteval decltype(auto) resolve_overload(auto&& overloaded) {
    return static_cast<std::add_pointer_t<Type>>((overloaded));
}

namespace cctype {

template <typename Ret, typename... ExtraArgs>
consteval auto make_safe_wrapper(int (&cfunc)(int, ExtraArgs...)) {
    return [&cfunc](char c, ExtraArgs... other_args) -> Ret {
        return static_cast<Ret>(std::invoke(cfunc, static_cast<unsigned char>(c), other_args...));
    };
}

constexpr bool islower(char c) {
    return in_range(c, 'a', 'z');
}

constexpr bool isupper(char c) {
    return in_range(c, 'A', 'Z');
}

constexpr char tolower(char c) {
    if (isupper(c)) {
        return c += ('a' - 'A');
    }
    return c;
}

constexpr char toupper(char c) {
    if (islower(c)) {
        return c -= ('a' - 'A');
    }
    return c;
}

constexpr bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool isxdigit(char c) {
    return isdigit(c) || in_range(tolower(c), 'a', 'f');
}

constexpr bool isalpha(char c) {
    return islower(c) || isupper(c);
}

constexpr bool isalnum(char c) {
    return isalpha(c) || isdigit(c);
}

// Using C doc table for ASCII values ranges
// See the table at the bottom of https://en.cppreference.com/w/cpp/string/byte/isgraph.html

constexpr bool iscntrl(char c) {
    return c <= '\x1F' || c == '\x7F';
}

constexpr bool isgraph(char c) {
    return detail::in_range(c, '\x20', '\x7E');
}

constexpr bool isprint(char c) {
    return isgraph(c) || c == ' ';
}

constexpr bool isblank(char c) {
    return c == '\t' || c == ' ';
}

constexpr bool isspace(char c) {
    return isblank(c) || detail::in_range(c, '\xA', '\xD');
}

constexpr bool ispunct(char c) {
    using detail::in_range;
    return in_range(c, '\x21', '\x2F') || in_range(c, '\x3A', '\x40') || in_range(c, '\x5B', '\x60') ||
           in_range(c, '\x7B', '\x7E');
}

} // namespace cctype

} // namespace detail

#define CCTYPE_IMPL(name, ret_type)                                                                                    \
    constexpr ret_type name(char c) {                                                                                  \
        if (std::is_constant_evaluated()) {                                                                            \
            return detail::cctype::name(c);                                                                            \
        }                                                                                                              \
        return detail::cctype::make_safe_wrapper<ret_type>(std::name)(c);                                              \
    }

CCTYPE_IMPL(isdigit, bool);
CCTYPE_IMPL(isxdigit, bool);
CCTYPE_IMPL(islower, bool);
CCTYPE_IMPL(isupper, bool);
CCTYPE_IMPL(isalpha, bool);
CCTYPE_IMPL(isalnum, bool);
CCTYPE_IMPL(iscntrl, bool);
CCTYPE_IMPL(isgraph, bool);
CCTYPE_IMPL(isprint, bool);
CCTYPE_IMPL(isblank, bool);
CCTYPE_IMPL(isspace, bool);
CCTYPE_IMPL(ispunct, bool);

CCTYPE_IMPL(toupper, char);
CCTYPE_IMPL(tolower, char);

#undef CCTYPE_IMPL

template <typename T>
    requires std::is_arithmetic_v<T>
constexpr T abs(T val) {
    if (std::is_constant_evaluated()) {
        if constexpr (std::integral<T> && std::is_signed_v<T>) {
            ASSERT(val != std::numeric_limits<T>::min(), "abs of signed minimum would be UB");
        }
        return (val < 0 ? -val : val);
    }

    return std::abs(val);
}

template <typename BaseT, std::unsigned_integral ExpT>
    requires std::is_arithmetic_v<BaseT>
constexpr std::common_type_t<BaseT, ExpT> pow(BaseT base, ExpT exp) {
    if (std::is_constant_evaluated()) {
        auto val = 1;
        while (exp-- > 0) {
            val *= base;
        }
        return val;
    }

    return std::pow(base, exp);
}

} // namespace asmgrader
