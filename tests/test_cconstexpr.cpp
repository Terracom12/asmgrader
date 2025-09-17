#include "catch2_custom.hpp"

#include "common/cconstexpr.hpp"
#include "common/static_string.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace asmgrader::literals;

constexpr asmgrader::StaticString xdigits = "0123456789ABCDEF";
constexpr asmgrader::StaticString digits = xdigits.remove_suffix<6>();

constexpr asmgrader::StaticString alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr asmgrader::StaticString alphabet_lower = alphabet.remove_suffix<26>();
constexpr asmgrader::StaticString alphabet_upper = alphabet.remove_prefix<26>();

constexpr asmgrader::StaticString blank = " \t";
constexpr asmgrader::StaticString space = blank + "\f\n\r\v";

constexpr asmgrader::StaticString punctuation = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)";
constexpr asmgrader::StaticString graphical = xdigits + alphabet + punctuation;
constexpr asmgrader::StaticString printable = graphical + " ";

constexpr asmgrader::StaticString control = "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF\x10\x11\x12\x13\x14\x15"
                                            "\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F";

TEST_CASE("cctype basic tests") {
    CONSTEXPR_RANGE_FOR(char c, xdigits) {
        STATIC_REQUIRE(asmgrader::isxdigit(c));
    };

    CONSTEXPR_RANGE_FOR(char c, digits) {
        STATIC_REQUIRE(asmgrader::isdigit(c));
        STATIC_REQUIRE(asmgrader::isalnum(c));

        // Check that toupper and tolower have no effect on non-alpha chars
        STATIC_REQUIRE(asmgrader::tolower(c) == c);
        STATIC_REQUIRE(asmgrader::toupper(c) == c);
    };

    CONSTEXPR_RANGE_FOR(char c, alphabet) {
        STATIC_REQUIRE(asmgrader::isalpha(c));
        STATIC_REQUIRE(asmgrader::isalnum(c));

        STATIC_REQUIRE(asmgrader::isupper(asmgrader::toupper(c)));
        STATIC_REQUIRE(asmgrader::islower(asmgrader::tolower(c)));
    };

    CONSTEXPR_RANGE_FOR(char c, alphabet_lower) {
        STATIC_REQUIRE(asmgrader::islower(c));
        STATIC_REQUIRE(asmgrader::tolower(c) == c);
        STATIC_REQUIRE_FALSE(asmgrader::isupper(c));
    };

    CONSTEXPR_RANGE_FOR(char c, alphabet_upper) {
        STATIC_REQUIRE_FALSE(asmgrader::islower(c));
        STATIC_REQUIRE(asmgrader::toupper(c) == c);
        STATIC_REQUIRE(asmgrader::isupper(c));
    };

    CONSTEXPR_RANGE_FOR(char c, punctuation) {
        STATIC_REQUIRE(asmgrader::ispunct(c));
    };

    CONSTEXPR_RANGE_FOR(char c, graphical) {
        STATIC_REQUIRE(asmgrader::isgraph(c));
    };

    CONSTEXPR_RANGE_FOR(char c, printable) {
        STATIC_REQUIRE(asmgrader::isprint(c));

        STATIC_REQUIRE_FALSE(asmgrader::iscntrl(c));
    };

    CONSTEXPR_RANGE_FOR(char c, blank) {
        STATIC_REQUIRE(asmgrader::isblank(c));
    };

    CONSTEXPR_RANGE_FOR(char c, space) {
        STATIC_REQUIRE(asmgrader::isspace(c));
    };

    CONSTEXPR_RANGE_FOR(char c, control) {
        STATIC_REQUIRE(asmgrader::iscntrl(c));

        STATIC_REQUIRE_FALSE(asmgrader::isprint(c));
    };
}
