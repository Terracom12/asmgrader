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
#include <fmt/format.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/copy_n.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace asmgrader {

/// A fully compile-time capable string type
/// Guaranteed to be null-terminated
template <std::size_t N>
class StaticString
{
public:
    constexpr StaticString() = default;

    // NOLINTNEXTLINE(google-explicit-constructor,*-avoid-c-arrays)
    constexpr /*implicit*/ StaticString(const char (&input)[N + 1]) {
        if (input[N] != '\0') {
            throw;
        }
        ranges::copy_n(std::data(input), N + 1, data.begin());
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr /*implicit*/ StaticString(const ranges::forward_range auto& rng) {
        ranges::copy_n(ranges::begin(rng), N + 1, data.begin());

        if (data.back() != '\0') {
            throw;
        }
    }

    std::array<char, N + 1> data{};

    constexpr std::size_t size() const { return data.size(); }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr /*implicit*/ operator std::string_view() const { return data.data(); }

    std::string str() const { return data.data(); }

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
    constexpr auto COMPILED_FMT = FMT_COMPILE(Fmt.data.begin());

    StaticString<MaxSz> result;

    fmt::format_to(result.data.begin(), COMPILED_FMT, std::forward<Args>(args)...);

    return result;
}

template <StaticString String>
consteval auto operator""_static() {
    return String;
}

} // namespace asmgrader

template<std::size_t N>
struct fmt::formatter<::asmgrader::StaticString<N>>  {
    fmt::formatter<std::string_view> underlying;


    constexpr auto parse(fmt::format_parse_context& ctx) {
        return underlying.parse(ctx);
    }

    constexpr auto format(const ::asmgrader::StaticString<N>& from, fmt::format_context& ctx) const {
        return underlying.format(from, ctx);
    }

};
