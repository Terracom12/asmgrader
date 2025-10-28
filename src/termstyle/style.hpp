/// \file
/// Defines the Style class.
#pragma once

#include "common/aliases.hpp"
#include "common/expected.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace asmgrader::termstyle {

/// Represents a terminal style including foreground and background colors, and emphasis.
/// Just a simple wrapper around fmt::text_style for the time being.
class Style
{
public:
    enum class Emphasis : u8 {
        None = 0,
        Bold = 1,
        Faint = 1 << 1,
        Italic = 1 << 2,
        Underline = 1 << 3,
        Blink = 1 << 4,
        Reverse = 1 << 5,
        Conceal = 1 << 6,
        Strikethrough = 1 << 7
    };

    struct Color
    {
        u8 r;
        u8 g;
        u8 b;

        constexpr Color() = default;

        explicit(false) constexpr Color(u32 rgb);
        constexpr Color(u8 r, u8 g, u8 b);
        explicit(false) constexpr Color(std::string_view hex_str);

        constexpr u32 rgb() const;
        static constexpr Expected<Color, std::errc> from_str(std::string_view hex_str);
    };

    static_assert(sizeof(Style::Color) == sizeof(u8) * 3);

    constexpr Style() = default;

    explicit constexpr Style(Color color, Emphasis emphasis = Emphasis::None);

    constexpr Color get_color() const;
    constexpr Emphasis get_emphasis() const;

    std::string apply();

private:
    Color color_{};
    Emphasis emphasis_{};
};

constexpr Style::Emphasis& operator|=(Style::Emphasis& lhs, Style::Emphasis rhs) {
    lhs = Style::Emphasis{static_cast<u8>(fmt::underlying(lhs) | fmt::underlying(rhs))};
    return lhs;
}

constexpr Style::Emphasis operator|(Style::Emphasis lhs, Style::Emphasis rhs) {
    return lhs |= rhs;
}

constexpr Style::Color::Color(u32 rgb)
    : r{static_cast<u8>(rgb & 0xFF0000)}
    , g{static_cast<u8>(rgb & 0x00FF00)}
    , b{static_cast<u8>(rgb & 0x0000FF)} {
    DEBUG_ASSERT((rgb & 0xFFFF0000) == 0, "rgb color value is too large");
}

constexpr Style::Color::Color(u8 r, u8 g, u8 b)
    : r{r}
    , g{g}
    , b{b} {};

constexpr Style::Color::Color(std::string_view hex_str)
    : Color(*from_str(hex_str).or_else(
          [](std::errc) -> u32 { throw std::runtime_error{"Error constructing Color from hex string"}; })) {}

constexpr u32 Style::Color::rgb() const {
    return static_cast<u32>(r + (g << 8) + (b << 16));
}

constexpr Expected<Style::Color, std::errc> Style::Color::from_str(std::string_view hex_str) {
    if (hex_str.starts_with('#')) {
        hex_str.remove_prefix(1);
    }
    if (hex_str.starts_with("0x") || hex_str.starts_with("0X")) {
        hex_str.remove_prefix(2);
    }

    u32 val{};
    auto from_chars_res = std::from_chars(hex_str.data(), hex_str.data() + hex_str.size(), val, 16);
    if (from_chars_res.ec != std::errc{}) {
        return from_chars_res.ec;
    }

    return val;
}

static_assert(Style::Color{"FF00FF"}.r == 0xFF);

constexpr Style::Color Style::get_color() const {
    return color_;
}

constexpr Style::Emphasis Style::get_emphasis() const {
    return emphasis_;
}

} // namespace asmgrader::termstyle
