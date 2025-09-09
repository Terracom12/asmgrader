#include "catch2_custom.hpp"

#include "common/cconstexpr.hpp"

TEST_CASE("cctype basic tests") {
    STATIC_REQUIRE(asmgrader::isdigit('0'));
}
