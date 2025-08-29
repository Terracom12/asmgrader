#include "catch2_custom.hpp"

#include "api/registers_state.hpp"
#include "common/aliases.hpp"

using namespace asmgrader::aliases;
using asmgrader::FloatingPointRegister, asmgrader::IntRegister;

static_assert(FloatingPointRegister<>::from(f16{1.0}).as<f16>() == 1.0F);
static_assert(FloatingPointRegister<>::from(1.0F).as<float>() == 1.0F);
static_assert(FloatingPointRegister<>::from(1.0F).as<float>() == 1.0F);
static_assert(FloatingPointRegister<>::from(1.0).as<double>() == 1.0);
static_assert(FloatingPointRegister<>::from(3.14) == 3.14);

static_assert(IntRegister<>{7} == 7);
// NOLINTBEGIN(readability-magic-numbers)
static_assert(IntRegister<>{0xFF}.as<i8>() == -1);
static_assert(IntRegister<>{0xFFFF}.as<i16>() == -1);
static_assert(IntRegister<>{0xFFFFFFFF}.as<i32>() == -1);
static_assert(IntRegister<>{0xFFFFFFFFFFFFFFFF}.as<i64>() == -1);
// NOLINTEND(readability-magic-numbers)
