/// \file
/// Syntax highlighting of expressions as generated in \ref expression_inspection.hpp
/// using {fmt}'s color API.
#pragma once

#include <asmgrader/api/expression_inspection.hpp>

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace asmgrader::highlight {

using Option = std::pair<inspection::Token::Kind, fmt::text_style>;

using enum inspection::Token::Kind;

struct Options
{
    static constexpr auto num_token_kinds = static_cast<std::size_t>(EndDelimiter);

    struct Opt
    {
        /// Style for the token kind
        fmt::text_style style;
        /// Function to call to determine the style for a token kind with a specific string.
        /// May be null. Overrides `style`.
        std::function<fmt::text_style(std::string_view)> style_fn;
        /// Function to call to transform a token string. May be null.
        std::function<std::string(std::string_view)> transform_fn;

        std::string apply(std::string_view str) const {
            fmt::text_style style_in_use = style;
            if (style_fn) {
                style_in_use = style_fn(str);
            }

            std::string res{str};

            if (transform_fn) {
                res = transform_fn(res);
            }

            return fmt::to_string(fmt::styled(res, style_in_use));
        }
    };

    /// Custom styling functions for identifiers that are types (e.g., void, int, etc.)
    static fmt::text_style style_basic_ident_types(fmt::text_style type_style, fmt::text_style default_style,
                                                   std::string_view ident) {
        std::array basic_type_strs{"void", "int", "long", "unsigned", "float", "double", "char", "size_t"};

        if (ranges::contains(basic_type_strs, ident)) {
            return type_style;
        }

        return default_style;
    }

    static fmt::text_style style_op_keywords(fmt::text_style keyword_style, fmt::text_style default_style,
                                             std::string_view op) {
        ASSERT(!op.empty(), op);

        if (isalpha(op.at(0))) {
            return keyword_style;
        }

        return default_style;
    }

    /// Add spacing for binary ops
    static std::string basic_binary_op_spacing(std::string_view op) {
        constexpr std::array add_spaces_surrounding{
            "+",  "-",  "*",   "/",  "%",                                        //
            "<<", ">>", "^",   "|",  "&",                                        //
            "&&", "||",                                                          //
            "==", "!=", "<=>", "<",  "<=", ">",  ">=",                           //
            "=",  "+=", "-=",  "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", //
            "?",  ":"                                                            //
        };

        if (ranges::contains(add_spaces_surrounding, op)) {
            return fmt::format(" {} ", op);
        }

        if (op == ",") {
            return fmt::format("{} ", op);
        }

        return std::string{op};
    }

    // Add some spacing around unary and ternary ops
    static std::string basic_op_spacing(std::string_view op) {
        constexpr std::array add_spaces_surrounding{
            "?",
            ":" //
        };
        constexpr std::array add_space_after{
            "throw", "sizeof", "alignof", "new", "delete" //
        };

        if (ranges::contains(add_spaces_surrounding, op)) {
            return fmt::format(" {} ", op);
        }

        if (ranges::contains(add_space_after, op)) {
            return fmt::format("{} ", op);
        }

        return std::string{op};
    }

    static Options get_default_options() {
        std::array<Opt, num_token_kinds> res{};

        using enum fmt::color;

        constexpr auto idx = [](inspection::Token::Kind kind) { return static_cast<std::size_t>(kind); };

        auto ident_style_fn = std::bind_front(style_basic_ident_types, fmt::fg(purple), fmt::fg(deep_sky_blue));
        auto op_style_fn = std::bind_front(style_op_keywords, fmt::fg(purple), fmt::text_style{});

        res[idx(StringLiteral)] = Opt{.style = fmt::fg(lawn_green), .style_fn = {}, .transform_fn = {}};
        res[idx(RawStringLiteral)] = Opt{.style = fmt::fg(lawn_green), .style_fn = {}, .transform_fn = {}};
        res[idx(BoolLiteral)] = Opt{.style = fmt::fg(purple), .style_fn = {}, .transform_fn = {}};
        res[idx(IntBinLiteral)] = res[idx(IntOctLiteral)] = res[idx(IntDecLiteral)] = res[idx(IntHexLiteral)] =
            Opt{.style = fmt::fg(azure), .style_fn = {}, .transform_fn = {}};
        res[idx(FloatLiteral)] = res[idx(FloatLiteral)] =
            Opt{.style = fmt::fg(blue), .style_fn = {}, .transform_fn = {}};
        res[idx(Identifier)] = Opt{.style = {}, .style_fn = ident_style_fn, .transform_fn = {}};
        res[idx(Operator)] = Opt{.style = {}, .style_fn = op_style_fn, .transform_fn = basic_op_spacing};
        res[idx(BinaryOperator)] = Opt{.style = {}, .style_fn = op_style_fn, .transform_fn = basic_binary_op_spacing};

        return {res};
    }

    /// Each index is representative of the highlighting option for the token kind
    /// enumerator with a value == index.
    /// e.g., token_opts[0] is the option for StringLiteral
    std::array<Opt, num_token_kinds> token_opts{};

    constexpr Opt& operator[](std::size_t idx) { return token_opts.at(idx); }

    constexpr Opt& operator[](inspection::Token::Kind kind) { return token_opts.at(static_cast<std::size_t>(kind)); }

    constexpr const Opt& operator[](std::size_t idx) const { return token_opts.at(idx); }

    constexpr const Opt& operator[](inspection::Token::Kind kind) const {
        return token_opts.at(static_cast<std::size_t>(kind));
    }
};

// If unspecified, uses the default of whatever text rendering method is in use

inline std::string highlight(const inspection::Token& token, const Options& opts = Options::get_default_options()) {
    return opts[token.kind].apply(token.str);
}

inline std::string highlight(const ranges::sized_range auto& tokens,
                             const Options& opts = Options::get_default_options()) {
    return tokens | ranges::views::transform([&opts](const inspection::Token& tok) { return highlight(tok, opts); }) |
           ranges::views::join | ranges::to<std::string>();
}

} // namespace asmgrader::highlight
