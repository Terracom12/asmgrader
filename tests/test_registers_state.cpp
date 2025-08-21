#include "catch2_custom.hpp"

#include "api/registers_state.hpp"

static_assert(sizeof(FloatingPointRegister) == 128 / 8);
static_assert(FloatingPointRegister::from(f16{1.0}).as<f16>() == 1.0);
static_assert(FloatingPointRegister::from(1.0F).as<float>() == 1.0F);
static_assert(FloatingPointRegister::from(1.0F).as<float>() == 1.0F);
static_assert(FloatingPointRegister::from(1.0).as<double>() == 1.0);
static_assert(FloatingPointRegister::from(3.14) == 3.14);

static_assert(sizeof(GeneralRegister) == 64 / 8);
static_assert(GeneralRegister{7} == 7);
// NOLINTBEGIN(readability-magic-numbers)
static_assert(GeneralRegister{0xFF}.as<std::int8_t>() == -1);
static_assert(GeneralRegister{0xFFFF}.as<std::int16_t>() == -1);
static_assert(GeneralRegister{0xFFFFFFFF}.as<std::int32_t>() == -1);
static_assert(GeneralRegister{0xFFFFFFFFFFFFFFFF}.as<std::int64_t>() == -1);
// NOLINTEND(readability-magic-numbers)
