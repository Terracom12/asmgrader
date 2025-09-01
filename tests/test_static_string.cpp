#include "catch2_custom.hpp"

#include "common/static_string.hpp"

#include <string_view>

using asmgrader::StaticString, asmgrader::static_format;

TEST_CASE("Static string construction and equality checks") {
    // GCC Bug until around version 14
    // See: https://godbolt.org/z/vKEPY1qWT
#if !defined(__GNUC__) || __GNUC__ >= 14
    STATIC_REQUIRE("abc" == std::string_view{StaticString("abc")});
#endif

    STATIC_REQUIRE(StaticString("abc") == "abc");
    STATIC_REQUIRE("abc" == StaticString("abc"));
    STATIC_REQUIRE("abc" == StaticString<3>(std::string_view{"abc"}));
}

TEST_CASE("Basic static_format checks") {
    STATIC_REQUIRE(static_format<"{0} + {0} = {1}">(1, 2) == "1 + 1 = 2");
    STATIC_REQUIRE(static_format<"">(1, 2) == "");
}
