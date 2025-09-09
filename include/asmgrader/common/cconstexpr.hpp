/// \file
/// constexpr capable functions from c-style headers
///
/// All functions in this file will fall back to the c-style implementations when not in a constant-evaluated context,
/// so runtime performance is guaranteed to be *at least* as good. Also fixes UB issues with `cctype` functions.
///
/// Refer to the [cppreference page on std lib headers](https://en.cppreference.com/w/cpp/headers.html) for the pages I
/// used as authoritative definitions in implementing these functions.

#include <asmgrader/common/aliases.hpp>

#include <libassert/assert.hpp>

#include <cctype>
#include <concepts>
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

template <std::invocable<int> auto Func>
consteval auto make_safe_cctype_wrapper() {
    return [](char c) { return static_cast<bool>(std::invoke(Func, static_cast<unsigned char>(c))); };
}

constexpr bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr char tolower(char c) {
    return c += ('a' - 'A');
}

constexpr char toupper(char c) {
    return c -= ('a' - 'A');
}

constexpr bool isxdigit(char c) {
    return isdigit(c) || in_range(tolower(c), 'a', 'f');
}

constexpr bool islower(char c) {
    return in_range(c, 'a', 'z');
}

constexpr bool isupper(char c) {
    return in_range(c, 'A', 'Z');
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
    return c <= '\x1F';
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

template <auto ConstexprFunc, auto CFunc, typename... Args>
consteval auto make_cfunc_wrapper() {
    return [](Args... args) {
        if (std::is_constant_evaluated()) {
            return std::invoke(ConstexprFunc, args...);
        }
        return std::invoke(CFunc, args...);
    };
}

template <template <typename...> typename ConstexprFunc, auto CFunc, typename... Args>
consteval auto make_cfunc_wrapper() {
    return [](Args... args) {
        if (std::is_constant_evaluated()) {
            return ConstexprFunc<Args...>(args...);
        }
        return std::invoke(CFunc, args...);
    };
}

} // namespace detail

CCTYPE_IMPL(isdigit);
CCTYPE_IMPL(isxdigit);
CCTYPE_IMPL(islower);
CCTYPE_IMPL(isupper);
CCTYPE_IMPL(isalpha);
CCTYPE_IMPL(isalnum);
CCTYPE_IMPL(iscntrl);
CCTYPE_IMPL(isgraph);
CCTYPE_IMPL(isprint);
CCTYPE_IMPL(isblank);
CCTYPE_IMPL(isspace);
CCTYPE_IMPL(ispunct);

#undef CCTYPE_IMPL

namespace detail::cmath {

template <typename T>
    requires std::is_arithmetic_v<T>
consteval T abs(T val) {
    if constexpr (std::integral<T> && std::is_signed_v<T>) {
        ASSERT(val != std::numeric_limits<T>::min(), "abs of signed minimum would be UB");
    }
    return (val < 0 ? -val : val);
}

template <std::integral BaseT, std::unsigned_integral ExpT>
consteval std::common_type_t<BaseT, ExpT> pow(BaseT base, ExpT exp) {
    auto val = 1;
    while (exp-- > 0) {
        val *= base;
    }
    return val;
}

} // namespace detail::cmath

constexpr auto(abs) = detail ::make_cfunc_wrapper<detail ::cmath ::abs, std ::abs, int>();

} // namespace asmgrader
