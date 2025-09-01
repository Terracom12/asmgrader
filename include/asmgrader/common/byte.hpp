#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/formatters/debug.hpp>

#include <fmt/base.h>
#include <fmt/format.h>
#include <gsl/util>

#include <concepts>
#include <cstddef>
#include <stdexcept>

namespace asmgrader {

/// More user-friendly interface wrapper for a byte-like integral
class Byte
{
public:
    constexpr Byte() = default;
    explicit(false) constexpr Byte(u8 val)
        : value{val} {};

    explicit(false) constexpr Byte(std::byte val)
        : Byte{static_cast<u8>(val)} {}

    /// Implicit *compile-time* narrowing conversion
    /// this means something like Byte{1000} would fail to compile,
    /// which is what one should expect.
    ///
    /// Wraps ``narrow_from``
    template <std::integral Narrowable>
        requires(!std::same_as<Narrowable, u8>)
    explicit(false) consteval Byte(Narrowable val)
        : value{narrow_from(val).value} {}

    /// Attempt to perform a narrowing conversion from val to a Byte.
    /// \throws out_of_range
    template <std::integral Narrowable>
    static constexpr Byte narrow_from(Narrowable val) {
        constexpr auto max_val = 255;

        if (val < 0 || val > max_val) {
            throw std::out_of_range("value is not representable as a byte");
        }

        return gsl::narrow_cast<u8>(val);
    }

    /// Explicit conversion to u8
    explicit constexpr operator u8() const { return value; }

    /// Explicit conversion to std::byte
    explicit constexpr operator std::byte() const { return std::byte{value}; }

    /// Compiler generated equality operator with another ``Byte``
    constexpr bool operator==(const Byte& rhs) const = default;

    /// Compiler generated comparison operators with another ``Byte``
    constexpr auto operator<=>(const Byte& rhs) const = default;

    // /// Equality operator with an integer type
    // /// Defined to prevent ambiguity with implicit conversions.
    // template <std::integral IntType>
    //     requires(!std::same_as<IntType, u8>)
    // constexpr bool operator==(Narrowable) const {};
    //
    // /// Comparison operators with an integer type
    // /// Defined to prevent ambiguity with implicit conversions.
    // template <std::integral IntType>
    //     requires(!std::same_as<IntType, u8>)
    // constexpr auto operator<=>(IntType) const {};

    /// Unary bitwise not
    constexpr Byte operator~() const { return static_cast<u8>(~value); }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_BINARY_OP(op)                                                                                              \
    constexpr Byte operator op(const Byte& rhs) const { return static_cast<u8>(value op rhs.value); }
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_BINARY_ASSIGN_OP(op)                                                                                       \
    constexpr Byte& operator op(const Byte & rhs) {                                                                    \
        value op rhs.value;                                                                                            \
        return *this;                                                                                                  \
    }

    /// Bitwise or
    DEF_BINARY_OP(|)
    /// Bitwise and
    DEF_BINARY_OP(&)
    /// Bitwise xor
    DEF_BINARY_OP(^)
    /// Bitwise right shift
    DEF_BINARY_OP(>>)
    /// Bitwise left shift
    DEF_BINARY_OP(<<)

    /// Bitwise or assign
    DEF_BINARY_ASSIGN_OP(|=)
    /// Bitwise and assign
    DEF_BINARY_ASSIGN_OP(&=)
    /// Bitwise xor assign
    DEF_BINARY_ASSIGN_OP(^=)
    /// Bitwise right shift assign
    DEF_BINARY_ASSIGN_OP(>>=)
    /// Bitwise left shift assign
    DEF_BINARY_ASSIGN_OP(<<=)

#undef DEF_BINARY_OP
#undef DEF_BINARY_ASSIGN_OP

    u8 value{};
};

static_assert(sizeof(Byte) == 1);

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::Byte> : ::asmgrader::DebugFormatter
{
    using UnderlyingT = decltype(::asmgrader::Byte::value);
    fmt::formatter<UnderlyingT> f;

    bool empty_format_spec{};

    constexpr auto parse(fmt::format_parse_context& ctx) {
        ctx.advance_to(::asmgrader::DebugFormatter::parse(ctx));
        if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
            empty_format_spec = true;
        }
        return f.parse(ctx);
    }

    constexpr auto format(const ::asmgrader::Byte& from, fmt::format_context& ctx) const {
        if (is_debug_format) {
            ctx.advance_to(fmt::format_to(ctx.out(), "Byte{{"));
        }

        // hex formatting by default when nothing else is specified
        if (empty_format_spec) {
            if (is_debug_format) {
                *ctx.out()++ = '0';
                *ctx.out()++ = 'x';
            }
            ctx.advance_to(fmt::format_to(ctx.out(), "{:02X}", from.value));
        } else {
            ctx.advance_to(f.format(from.value, ctx));
        }

        if (is_debug_format) {
            *ctx.out()++ = '}';
        }

        return ctx.out();
    }
};
