#include "catch2_custom.hpp"

#include "common/expected.hpp"

#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <type_traits>

using namespace std::literals;
using asmgrader::Expected;

// TODO: regression test for operator== ambiguity with derived classes
// may still be present for <=>?

// Simple types
using Et = Expected<int, std::string>;

TEST_CASE("Simple construction and value checks") {
    // Default constructed "void-typed"
    REQUIRE(Expected{}.has_value());
    REQUIRE(!Expected{}.has_error());

    // REQUIRE_THROWS_AS(Expected{}.error(), AssertionError);

    REQUIRE(Et{123}.has_value());
    REQUIRE(!Et{123}.has_error());

    REQUIRE(!Et{"Hello"}.has_value());
    REQUIRE(Et{"Hello"}.has_error());
}

TEST_CASE("Equality and comparison operators") {
    REQUIRE(Expected{} == Expected{});

    REQUIRE(Et{123} == Et{123});
    REQUIRE(Et{123} != Et{456});
    REQUIRE(Et{123} <= Et{456});
    REQUIRE(Et{123} < Et{456});
    REQUIRE(Et{123} >= Et{123});
    REQUIRE(Et{456} > Et{123});

    // Implicit conversions from value / error
    REQUIRE(Et{123} == 123);
    REQUIRE(Et{123} != 456);
    REQUIRE(Et{123} != "1234");

    REQUIRE(Et{"Unexpected!"} == "Unexpected!");
    REQUIRE(Et{"Unexpected!"} != "Exp!");
    REQUIRE(Et{"Unexpected!"} != 12345);
}

TEST_CASE("Other (monadic) operations") {
    REQUIRE(Et{123}.value_or(456) == 123);
    REQUIRE(Et{123}.error_or("E") == "E");

    REQUIRE(Et{"A"}.value_or(123.5) == 123);
    REQUIRE(Et{"A"}.error_or("B") == "A");

    auto square = [](int n) { return n * n; };

    REQUIRE(Et{123}.transform(square) == 123 * 123);
    REQUIRE(Et{"no"}.transform(square) == "no");
}
