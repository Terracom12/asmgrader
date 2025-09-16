#pragma once

#include <concepts>

namespace asmgrader {

// Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97930 for gcc versions < 12.2
// *Partially* conformant to std::pair spec https://en.cppreference.com/w/cpp/utility/pair.html
template <typename T1, typename T2>
struct pair // NOLINT(readability-identifier-naming)
{
    constexpr pair() = default;

    constexpr pair(const T1& x, const T2& y)
        : first{x}
        , second{y} {}

    template <typename U1, typename U2>
        requires std::constructible_from<T1, U1> && std::constructible_from<T2, U2>
    constexpr pair(U1&& x, U2&& y)
        : first{std::forward<U1>(x)}
        , second{std::forward<U2>(y)} {}

    constexpr pair(const pair&) = default;
    constexpr pair(pair&&) = default;
    constexpr pair& operator=(const pair&) = default;
    constexpr pair& operator=(pair&&) = default;
    constexpr ~pair() = default;

    template <typename U1, typename U2>
        requires std::constructible_from<T1, const U1&> && std::constructible_from<T2, const U2&>
    constexpr explicit(false) pair(const pair<U1, U2>& other)
        : first{other.first}
        , second{other.second} {}

    template <typename U1, typename U2>
        requires std::constructible_from<T1, U1> && std::constructible_from<T2, U2>
    constexpr explicit(false) pair(pair<U1, U2>&& other)
        : first{std::forward<U1>(other.first)}
        , second{std::forward<U2>(other.second)} {}

    template <typename U1, typename U2>
        requires std::constructible_from<T1, const U1> && std::constructible_from<T2, const U2>
    constexpr explicit(false) pair(const pair<U1, U2>&& other)
        : first{std::forward<const U1>(other.first)}
        , second{std::forward<const U2>(other.second)} {}

    T1 first{};
    T2 second{};

    constexpr bool operator==(const pair& rhs) const = default;
    constexpr auto operator<=>(const pair& rhs) const = default;
};

template <typename T1, typename T2>
pair(T1, T2) -> pair<T1, T2>;

} // namespace asmgrader
