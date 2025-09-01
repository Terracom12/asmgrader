#include "catch2_custom.hpp"

#include "api/registers_state.hpp"
#include "common/aliases.hpp"

using namespace asmgrader::aliases;
using asmgrader::FloatingPointRegister, asmgrader::IntRegister;

TEST_CASE("FloatingPoint register from, as, and operator==") {
    STATIC_REQUIRE(FloatingPointRegister<>::from(f16{1.0}).as<f16>() == 1.0F);
    STATIC_REQUIRE(FloatingPointRegister<>::from(1.0F).as<float>() == 1.0F);
    STATIC_REQUIRE(FloatingPointRegister<>::from(1.0F).as<float>() == 1.0F);
    STATIC_REQUIRE(FloatingPointRegister<>::from(1.0).as<double>() == 1.0);
    STATIC_REQUIRE(FloatingPointRegister<>::from(3.14) == 3.14);
}

TEST_CASE("IntRegister from, as, and operator==") {
    STATIC_REQUIRE(IntRegister<>{7} == 7);
    // NOLINTBEGIN(readability-magic-numbers)
    STATIC_REQUIRE(IntRegister<>{0xFF}.as<i8>() == -1);
    STATIC_REQUIRE(IntRegister<>{0xFFFF}.as<i16>() == -1);
    STATIC_REQUIRE(IntRegister<>{0xFFFFFFFF}.as<i32>() == -1);
    STATIC_REQUIRE(IntRegister<>{0xFFFFFFFFFFFFFFFF}.as<i64>() == -1);
    // NOLINTEND(readability-magic-numbers)
}
