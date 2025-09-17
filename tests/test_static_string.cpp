#include "catch2_custom.hpp"

#include "common/static_string.hpp"

#include <fmt/base.h>
#include <fmt/compile.h>
#include <fmt/ranges.h>

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

TEST_CASE("Static string substr and remove_prefix/suffix") {
    STATIC_REQUIRE(StaticString{"abcd"}.substr<1>() == "bcd");
    STATIC_REQUIRE(StaticString{"abcd"}.substr<0, 1>() == "a");
    STATIC_REQUIRE(StaticString{"abcd"}.substr<0, 2>() == "ab");
    STATIC_REQUIRE(StaticString{"abcd"}.substr<0, 4>() == "abcd");
    STATIC_REQUIRE(StaticString{"abcd"}.substr<0, 5>() == "abcd");
    STATIC_REQUIRE(StaticString{"abcd"}.substr<0, 100>() == "abcd");

    STATIC_REQUIRE(StaticString{""}.substr<0>() == "");
    STATIC_REQUIRE(StaticString{""}.substr<0, 100>() == "");

    STATIC_REQUIRE(StaticString{""}.remove_prefix<0>() == "");
    STATIC_REQUIRE(StaticString{"abcd"}.remove_prefix<0>() == "abcd");
    STATIC_REQUIRE(StaticString{"[abcd"}.remove_prefix<1>() == "abcd");
    STATIC_REQUIRE(StaticString{"[[[abcd"}.remove_prefix<3>() == "abcd");

    STATIC_REQUIRE(StaticString{""}.remove_suffix<0>() == "");
    STATIC_REQUIRE(StaticString{"abcd"}.remove_suffix<0>() == "abcd");
    STATIC_REQUIRE(StaticString{"abcd]"}.remove_suffix<1>() == "abcd");
    STATIC_REQUIRE(StaticString{"abcd]]]"}.remove_suffix<3>() == "abcd");

    STATIC_REQUIRE(StaticString{"[abcd]"}.remove_prefix<1>().remove_suffix<1>() == "abcd");
    STATIC_REQUIRE(StaticString{"[[[abcd]]]"}.remove_prefix<3>().remove_suffix<3>() == "abcd");
}

TEST_CASE("Basic static_format checks") {
    STATIC_REQUIRE(static_format<"{0} + {0} = {1}">(1, 2) == "1 + 1 = 2");
    STATIC_REQUIRE(static_format<"">(1, 2) == "");
}

TEST_CASE("Static string is formattable") {
    static_assert(fmt::formattable<asmgrader::StaticString<1>>);

    constexpr StaticString a = [] {
        StaticString<10> a{};
        fmt::format_to(a.begin(), FMT_COMPILE("{}"), StaticString("Hello!"));
        return a;
    }();

    STATIC_REQUIRE(a == "Hello!");
}

// TODO: More tests
