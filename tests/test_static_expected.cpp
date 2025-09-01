#include "catch2_custom.hpp"

#include "common/expected.hpp"

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

using namespace std::literals;
using asmgrader::Expected;

// Simple types
using Et = Expected<int, std::string>;

TEST_CASE("Expected compile-time tests") {
    STATIC_REQUIRE(std::is_default_constructible_v<Expected<>>);

    STATIC_REQUIRE(Expected<int, const char*>(100) == 100);
    STATIC_REQUIRE(Expected<int, const char*>("ERROR") == "ERROR"sv);

    STATIC_REQUIRE(Expected<int, const char*>(100).error_or("No error") == "No error"sv);
    STATIC_REQUIRE(Expected<int, const char*>("ERROR").value_or(-1) == -1);

    STATIC_REQUIRE(Expected<void, const char*>().error_or("No error") == "No error"sv);
    STATIC_REQUIRE(Expected<void, const char*>("ERROR").error() == "ERROR"sv);

    constexpr auto aggregate_expected_test = [](bool error) -> Expected<std::array<int, 10>, int> {
        if (error) {
            return -1;
        }

        return std::array<int, 10>{1, 2, 3, 4, 5};
    };

    STATIC_REQUIRE(aggregate_expected_test(false).value() == std::array<int, 10>{1, 2, 3, 4, 5});
    STATIC_REQUIRE(aggregate_expected_test(true).error() == -1);
}
