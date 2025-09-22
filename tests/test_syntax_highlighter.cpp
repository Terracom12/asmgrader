#include "catch2_custom.hpp"

#include "api/syntax_highlighter.hpp"

#include <fmt/color.h>
#include <fmt/format.h>

#include <string>
#include <string_view>

using asmgrader::highlight::render_blocks;

namespace {

std::string st(std::string_view str, fmt::text_style style) {
    return fmt::to_string(fmt::styled(str, style));
}

} // namespace

using enum fmt::terminal_color;
using fmt::rgb;
using enum fmt::emphasis;

TEST_CASE("Basic literal block rendering") {
    // Very basic singular blocks
    REQUIRE(render_blocks("%#``", true) == "");
    REQUIRE(render_blocks("%#`a`", true) == "a");
    REQUIRE(render_blocks("%#`abc`", true) == "abc");

    // Strings with no literal blocks
    REQUIRE(render_blocks("", true) == "");
    REQUIRE(render_blocks("`", true) == "`");
    REQUIRE(render_blocks("``", true) == "``");
    REQUIRE(render_blocks(" #`abc `", true) == " #`abc `");

    // Escaping `
    REQUIRE(render_blocks("%#`\\``", true) == "`");
    REQUIRE(render_blocks("%#`abc\\` `", true) == "abc` ");
    REQUIRE(render_blocks("%#`example: \\`some code int main() {}\\``", true) == "example: `some code int main() {}`");

    // Multiple literal blocks
    REQUIRE(render_blocks("%#``%#``", true) == "");
    REQUIRE(render_blocks("%#`a`%#``", true) == "a");
    REQUIRE(render_blocks("%#``%#`1`", true) == "1");
    REQUIRE(render_blocks("%#`a`%#`1`", true) == "a1");
    REQUIRE(render_blocks("%#`abc`%#`123`", true) == "abc123");

    // Non block strings and block strings together
    REQUIRE(render_blocks("%#`123`abc", true) == "123abc");
    REQUIRE(render_blocks("abc%#`123`", true) == "abc123");
    REQUIRE(render_blocks("%#`123`abc%#`456`", true) == "123abc456");
    REQUIRE(render_blocks("%#`123\\`` \\`abc ` %#``%#`FFF`%#`` ", true) == "123` \\`abc ` FFF ");
}

TEST_CASE("Literal block style spec parsing") {
    /// - `fg:ansi-name` | `bg:ansi-name` - where **ansi-name** is one of: black, red, green, yellow, blue, magenta,
    /// cyan, or white
    /// - `fg:#hex-color` | `bg:#hex-color` - where **hex-color** is a 6-digit hex rgb color value (e.g., red foreground
    /// `fg:#FF0000`)
    /// - `bold` | `italic` | `underline` | `blink` | `strikethrough`

    REQUIRE(render_blocks("%#`<>`", false) == "<>");

    REQUIRE(render_blocks("%#`<bold>`", false) == st("", bold));
    REQUIRE(render_blocks("%#`<italic>`", false) == st("", italic));
    REQUIRE(render_blocks("%#`<underline>`", false) == st("", underline));
    REQUIRE(render_blocks("%#`<blink>`", false) == st("", blink));
    REQUIRE(render_blocks("%#`<strikethrough>`", false) == st("", strikethrough));

    REQUIRE(render_blocks("%#`<fg:black>`", false) == st("", fg(black)));
    REQUIRE(render_blocks("%#`<fg:red>`", false) == st("", fg(red)));
    REQUIRE(render_blocks("%#`<fg:green>`", false) == st("", fg(green)));
    REQUIRE(render_blocks("%#`<fg:yellow>`", false) == st("", fg(yellow)));
    REQUIRE(render_blocks("%#`<bg:blue>`", false) == st("", bg(blue)));
    REQUIRE(render_blocks("%#`<bg:magenta>`", false) == st("", bg(magenta)));
    REQUIRE(render_blocks("%#`<bg:cyan>`", false) == st("", bg(cyan)));
    REQUIRE(render_blocks("%#`<bg:white>`", false) == st("", bg(white)));

    REQUIRE(render_blocks("%#`<fg:#000000>`", false) == st("", fg(rgb(0x000000))));
    REQUIRE(render_blocks("%#`<bg:#FFFFFF>`", false) == st("", bg(rgb(0xFFFFFF))));

    REQUIRE(render_blocks("%#`<bg:#123456>`", false) == st("", bg(rgb(0x123456))));
    REQUIRE(render_blocks("%#`<bg:#ABCDEF>`", false) == st("", bg(rgb(0xABCDEF))));
    REQUIRE(render_blocks("%#`<bg:#56789A>`", false) == st("", bg(rgb(0x56789A))));

    REQUIRE(render_blocks("%#`<bg:#abcdef>`", false) == st("", bg(rgb(0xABCDEF))));

    REQUIRE(render_blocks("%#`<bold,italic>`", false) == st("", bold | italic));
    REQUIRE(render_blocks("%#`<bold,fg:red,bg:black>`", false) == st("", bold | fg(red) | bg(black)));
    REQUIRE(render_blocks("%#`<bold,italic,blink,fg:#123456,bg:#AAAAAA>`", false) ==
            st("", bold | italic | blink | fg(rgb(0x123456)) | bg(rgb(0xAAAAAA))));

    REQUIRE_THROWS_AS(render_blocks("%#`<fg:red,fg:black>`", false), fmt::format_error);
    REQUIRE_THROWS_AS(render_blocks("%#`<fg:red,fg:#000000>`", false), fmt::format_error);
    REQUIRE_THROWS_AS(render_blocks("%#`<fg:#000000,fg:red>`", false), fmt::format_error);

    REQUIRE_THROWS_AS(render_blocks("%#`<bg:red,bg:black>`", false), fmt::format_error);
    REQUIRE_THROWS_AS(render_blocks("%#`<bg:red,bg:#000000>`", false), fmt::format_error);
    REQUIRE_THROWS_AS(render_blocks("%#`<bg:#000000,bg:red>`", false), fmt::format_error);
}

TEST_CASE("Literal block styling use cases") {
    REQUIRE(render_blocks("%#`<fg:red,bg:yellow>I am an error!`", false) == st("I am an error!", fg(red) | bg(yellow)));
    REQUIRE(render_blocks("%#`<fg:blue>cool.` hi", false) == st("cool.", fg(blue)) + " hi");
}
