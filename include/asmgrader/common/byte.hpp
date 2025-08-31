#pragma once

#include <asmgrader/common/aliases.hpp>

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
