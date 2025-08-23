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

#include <fmt/base.h>
#include <fmt/compile.h>
#include <range/v3/algorithm/copy_n.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <string_view>

/// A fully compile-time capable string type
/// Guaranteed to be null-terminated
template <std::size_t N>
class StaticString
{
public:
    consteval StaticString() = default;

    // NOLINTNEXTLINE(google-explicit-constructor,*-avoid-c-arrays)
    consteval /*implicit*/ StaticString(const char (&input)[N + 1]) {
        if (input[N] != '\0') {
            throw;
        }
        ranges::copy_n(std::data(input), N + 1, data.begin());
    }

    std::array<char, N + 1> data{};

    // NOLINTNEXTLINE(google-explicit-constructor)
    consteval /*implicit*/ operator std::string_view() const { return {data.begin(), N}; }

    template <std::size_t OtherSize>
    consteval auto operator<=>(const StaticString<OtherSize>& rhs) const {
        return std::string_view{*this} <=> std::string_view{rhs};
    }

    // Support comparison with string_view-convertable objects
    constexpr auto operator<=>(std::string_view rhs) const { return std::string_view{*this} <=> rhs; }

    template <std::size_t OtherSize>
    consteval bool operator==(const StaticString<OtherSize>& rhs) const {
        return std::string_view{*this} == std::string_view{rhs};
    }

    // Support comparison with string_view-convertable objects
    consteval bool operator==(std::string_view rhs) const { return std::string_view{*this} == rhs; }
};

// Deduction guide
template <std::size_t N>
// NOLINTNEXTLINE(*-avoid-c-arrays)
StaticString(const char (&input)[N]) -> StaticString<N - 1>;

static_assert(StaticString("abc") == "abc");
static_assert("abc" == StaticString("abc"));

template <StaticString Fmt, fmt::formattable auto... Args>
consteval auto format_static() {
    constexpr auto COMPILED_FMT = FMT_COMPILE(Fmt.data.begin());
    constexpr std::size_t SIZE = fmt::formatted_size(COMPILED_FMT, Args...);

    StaticString<SIZE> result;

    fmt::format_to(result.data.begin(), COMPILED_FMT, Args...);

    return result;
}

static_assert(format_static<"{0} + {0} = {1}", 1, 2>() == "1 + 1 = 2");
static_assert(format_static<"", 1, 2>() == "");
