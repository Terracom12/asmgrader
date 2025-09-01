#include "catch2_custom.hpp"

#include "common/aliases.hpp"
#include "common/byte.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <type_traits>

using asmgrader::Byte;
using namespace asmgrader::aliases;

namespace {

// Testing via constexpr `Byte::narrow_from`, since it's pretty much impossible
// to test invalid cases for the consteval ctor, and that ctor is defined to wrap
// ``narrow_from`` function anyways

template <std::integral auto Int, typename Enable = void>
struct can_construct
{
    static constexpr bool value = false;
};

template <std::integral auto Int>
struct can_construct<Int, std::enable_if_t<Byte::narrow_from(Int).value || true>>
{
    static constexpr bool value = true;
};

template <std::integral auto Int>
constexpr bool can_construct_v = can_construct<Int>::value;

} // namespace

TEST_CASE("Byte construction") {
    STATIC_REQUIRE(Byte{u8{0x12}}.value == 0x12);

    STATIC_REQUIRE(Byte{std::byte{0xAA}}.value == 0xAA);

    STATIC_REQUIRE(Byte{0x0}.value == 0x0);
    STATIC_REQUIRE(Byte{0x1}.value == 0x1);
    STATIC_REQUIRE(Byte{0xF}.value == 0xF);
    STATIC_REQUIRE(Byte{0xFE}.value == 0xFE);
    STATIC_REQUIRE(Byte{0xFF}.value == 0xFF);

    STATIC_REQUIRE_FALSE(can_construct_v<0x100U>);
    STATIC_REQUIRE_FALSE(can_construct_v<0x101L>);
    STATIC_REQUIRE_FALSE(can_construct_v<0xFFFLL>);
    STATIC_REQUIRE_FALSE(can_construct_v<-1>);
    STATIC_REQUIRE_FALSE(can_construct_v<-2>);
    STATIC_REQUIRE_FALSE(can_construct_v<-10000>);
    STATIC_REQUIRE_FALSE(can_construct_v<1000000000000000ULL>);
}

TEST_CASE("Byte comparison operators") {
    // Compiler will not automatically convert an integer to a Byte
    // if the rhs is not a Byte. Without the extra () the rhs would
    // be Catch2's decomposer type'

    STATIC_REQUIRE((Byte{0x0} == 0x0));
    STATIC_REQUIRE((Byte{0xA} == 0xALL));
    STATIC_REQUIRE((Byte{0xFF} == 0xFFU));

    STATIC_REQUIRE((Byte{0x0} <= 0x0));
    STATIC_REQUIRE((Byte{0x0} >= 0x0));
    STATIC_REQUIRE((Byte{0x0} < 0xA));
    STATIC_REQUIRE((Byte{0xFF} > 0xA));
    STATIC_REQUIRE((Byte{0xFF} <=> 0xA) == std::strong_ordering::greater);
}

TEST_CASE("Byte bitwise operators") {
    // Compiler will not automatically convert an integer to a Byte
    // if the rhs is not a Byte. Without the extra () the rhs would
    // be Catch2's decomposer type'

    STATIC_REQUIRE((~Byte{0x0} == 0xFF));
    STATIC_REQUIRE(((Byte{0x1} | 0x2) == 0x3));
    STATIC_REQUIRE(((Byte{0x3} & 0x7) == 0x3));
    STATIC_REQUIRE(((Byte{0x3} ^ 0x5) == 0x6));
    STATIC_REQUIRE(((Byte{0xF0} >> 0x4) == 0xF));
    STATIC_REQUIRE(((Byte{0xF} << 0x4) == 0xF0));

    // (defined) shift over and underflow
    STATIC_REQUIRE(((Byte{0x80} >> 0x7) == 0x1));
    STATIC_REQUIRE(((Byte{0x3} << 0x7) == 0x80));

    STATIC_REQUIRE(([] {
        Byte b{0x1};
        b |= 0x2;
        return b;
    }() == 0x3));
    STATIC_REQUIRE(([] {
        Byte b{0x3};
        b &= 0x7;
        return b;
    }() == 0x3));
    STATIC_REQUIRE(([] {
        Byte b{0x3};
        b ^= 0x5;
        return b;
    }() == 0x6));
    STATIC_REQUIRE(([] {
        Byte b{0xF0};
        b >>= 0x4;
        return b;
    }() == 0xF));
    STATIC_REQUIRE(([] {
        Byte b{0xF};
        b <<= 0x4;
        return b;
    }() == 0xF0));

    // (defined) shift over and underflow
    STATIC_REQUIRE(([] {
        Byte b{0x80};
        b >>= 0x7;
        return b;
    }() == 0x1));
    STATIC_REQUIRE(([] {
        Byte b{0x3};
        b <<= 0x7;
        return b;
    }() == 0x80));
}
