// Heavily inspired by: https://blog.ganets.ky/StaticString/
// In accordance with above mentioned site's licensing:
//
// The MIT License (MIT)
// Copyright (c) 2013-2018 Blackrock Digital LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <asmgrader/meta/static_size.hpp>

#include <fmt/base.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/copy_if.hpp>
#include <range/v3/algorithm/copy_n.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/subrange.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace asmgrader {

/// A fully compile-time capable string type
/// Guaranteed to be null-terminated
template <std::size_t Size>
class StaticString
{
public:
    constexpr StaticString() = default;

    // NOLINTNEXTLINE(google-explicit-constructor,*-avoid-c-arrays)
    constexpr /*implicit*/ StaticString(const char (&input)[Size + 1]) {
        if (input[Size] != '\0') {
            throw;
        }
        ranges::copy_n(std::data(input), Size + 1, data.begin());
    }

    /// Do not include '\0'
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr /*implicit*/ StaticString(const ranges::forward_range auto& rng) {
        ranges::copy_n(ranges::begin(rng), Size, data.begin());

        data.back() = '\0';
    }

    std::array<char, Size + 1> data{};

    template <std::size_t Pos, std::size_t Len = std::numeric_limits<std::size_t>::max()>
        requires(Pos <= Size)
    constexpr auto substr() const {
        // exclude '\0' from ctor call
        constexpr auto end = std::min(Size, Pos + Len);
        constexpr auto substr_size = std::min(Size - Pos, Len);

        return StaticString<substr_size>{ranges::subrange{data.begin() + Pos, data.begin() + end}};
    }

    template <std::size_t N>
        requires(N <= Size)
    constexpr StaticString<Size - N> remove_prefix() const {
        // exclude '\0' from ctor call
        return {ranges::subrange{data.begin() + N, data.end() - 1}};
    }

    template <std::size_t N>
        requires(N <= Size)
    constexpr StaticString<Size - N> remove_suffix() const {
        // exclude '\0' from ctor call
        return {ranges::subrange{data.begin(), data.end() - 1 - N}};
    }

    template <typename Range>
    constexpr auto operator+(const Range& rhs) const {
        constexpr auto n = [] {
            constexpr auto rng_sz = get_static_size<Range>();
            using std::get;
            if constexpr (std::is_array_v<std::remove_cvref_t<Range>>) {
                return rng_sz - 1;
            }
            return rng_sz;
        }();

        StaticString<Size + n> result;

        ranges::copy_n(data.begin(), Size, result.data.begin());

        if constexpr (std::same_as<Range, StaticString<n>>) {
            ranges::copy_n(rhs.data.begin(), n, result.data.begin() + Size);
        } else {
            ranges::copy_n(ranges::begin(rhs), n, result.data.begin() + Size);
        }

        return result;
    }

    constexpr std::size_t size() const { return Size; }

    constexpr auto begin() { return data.begin(); }

    constexpr auto begin() const { return data.cbegin(); }

    constexpr auto cbegin() const { return data.cbegin(); }

    constexpr auto end() { return data.end() - 1; }

    constexpr auto end() const { return data.cend() - 1; }

    constexpr auto cend() const { return data.cend() - 1; }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr /*implicit*/ operator std::string_view() const { return data.data(); }

    std::string str() const { return data.data(); }

    constexpr char operator[](std::size_t i) const {
        ASSERT(i < Size);
        return data[i];
    }

    constexpr char& operator[](std::size_t i) {
        ASSERT(i < Size);
        return data[i];
    }

    constexpr char at(std::size_t i) const { return data.at(i); }

    constexpr char& at(std::size_t i) { return data.at(i); }

    template <std::size_t OtherSize>
    constexpr auto operator<=>(const StaticString<OtherSize>& rhs) const {
        return std::string_view{*this} <=> std::string_view{rhs};
    }

    // Support comparison with string_view-convertable objects
    constexpr auto operator<=>(std::string_view rhs) const { return std::string_view{*this} <=> rhs; }

    template <std::size_t OtherSize>
    constexpr bool operator==(const StaticString<OtherSize>& rhs) const {
        return std::string_view{*this} == std::string_view{rhs};
    }

    // Support comparison with string_view-convertable objects
    constexpr bool operator==(std::string_view rhs) const { return std::string_view{*this} == rhs; }
};

// Deduction guide
template <std::size_t N>
// NOLINTNEXTLINE(*-avoid-c-arrays)
StaticString(const char (&input)[N]) -> StaticString<N - 1>;

template <StaticString Fmt, std::size_t MaxSz = 10 * 1'024, fmt::formattable... Args>
constexpr auto static_format(Args&&... args) {
    constexpr auto compiled_fmt = FMT_COMPILE(Fmt.data.begin());

    StaticString<MaxSz> result;

    fmt::format_to(result.data.begin(), compiled_fmt, std::forward<Args>(args)...);

    return result;
}

template <std::size_t N>
constexpr std::string_view format_as(const StaticString<N>& str) {
    return str;
}

namespace literals {

template <StaticString String>
consteval auto operator""_static() {
    return String;
}

} // namespace literals

} // namespace asmgrader

namespace std {

/// Specialization of tuple_size to play nice with algorithms that work on tuple-like types
template <std::size_t Size>
// See: https://en.cppreference.com/w/cpp/utility/tuple_size.html
// NOLINTNEXTLINE(cert-dcl58-cpp) - this is well defined and probably a clang-tidy bug
struct tuple_size<::asmgrader::StaticString<Size>> : public std::integral_constant<std::size_t, Size>
{
};

/// Specialization of tuple_element to play nice with algorithms that work on tuple-like types
template <std::size_t I, std::size_t Size>
    requires(I < Size)
// See: https://en.cppreference.com/w/cpp/container/array/tuple_element.html
// NOLINTNEXTLINE(cert-dcl58-cpp) - this is well defined and probably a clang-tidy bug
struct tuple_element<I, ::asmgrader::StaticString<Size>>
{
    using type = char;
};

} // namespace std

namespace asmgrader {

/// Specialization of get to play nice with algorithms that work on tuple-like types
template <std::size_t I, std::size_t Size>
    requires(I < Size)
constexpr char get(const ::asmgrader::StaticString<Size>& str) {
    return str.at(I);
}

} // namespace asmgrader

// Testbed for {fmt} stuff as it's been really annoying to fix this

// Disable range formatting for StaticString to prevent ambiguity
template <std::size_t N, typename CharType>
struct fmt::is_range<asmgrader::StaticString<N>, CharType> : std::false_type
{
};

// Disable tuple formatting for StaticString to prevent ambiguity
template <std::size_t N>
struct fmt::is_tuple_like<asmgrader::StaticString<N>> : std::false_type
{
};
