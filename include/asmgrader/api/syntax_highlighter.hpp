/// \file
/// Syntax highlighting of expressions as generated in \ref expression_inspection.hpp
/// using {fmt}'s color API.
#pragma once

#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/logging.hpp>

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/split_when.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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

        std::string apply(std::string_view str) const;
    };

    /// Custom styling functions for identifiers that are types (e.g., void, int, etc.)
    static fmt::text_style style_basic_ident_types(fmt::text_style type_style, fmt::text_style default_style,
                                                   std::string_view ident);
    static fmt::text_style style_op_keywords(fmt::text_style keyword_style, fmt::text_style default_style,
                                             std::string_view op);
    /// Add spacing for binary ops
    static std::string basic_binary_op_spacing(std::string_view op);
    // Add some spacing around unary and ternary ops
    static std::string basic_op_spacing(std::string_view op);

    static Options get_default_options();

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

/// Parses and renders literal blocks, returning a form meant for displaying to a console user, as by means of
/// fmt::styled. The syntax for a literal string is a sequence of characters started by the sequence <code>%#\`</code>
/// and ended by a single backtick <code>\`</code>. Backticks within the sequence may be escaped using
/// <code>\\\`</code>.
///
/// This plain form is not very useful here, but is handy to disable tokenization with \ref highlight(std::string_view,
/// const Options&).
///
/// \param str The string to parse literal blocks from
/// \param skip_styling Whether to skip all styling.
///
/// Example, with possible output from \ref highlight(std::string_view, const Options&):
/// \verbatim
/// %#`I am a literal block with a some \`backticks!\``
/// int main() { return 0; }  %#`hey, this is main!`
/// \endverbatim
/// <pre>
/// I am a literal block with a some `backticks!`
/// <span style="color:purple">int</span> <span style="color:blue">main</span>() { <span
/// style="color:orange">return</span> 0; }\endcode  hey, this is main!
/// </pre>
///
/// ---
///
/// A style may also be specified within each literal block with the syntax `<style-name>` positioned at the
/// very beginning of the block. Multiple `style-name` fields may be seperated by `,`s **without spaces**.
/// To begin a block with a literal `<` character, escape it with a `\`.
/// Supported style names are:
/// - `fg:ansi-name` | `bg:ansi-name` - where **ansi-name** is one of: black, red, green, yellow, blue, magenta,
/// cyan, or white
/// - `fg:#hex-color` | `bg:#hex-color` - where **hex-color** is a 6-digit hex rgb color value (e.g., red foreground
/// `fg:#FF0000`)
/// - `bold` | `italic` | `underline` | `blink` | `strikethrough`
///
/// Example:
/// \verbatim
/// I am a not formatted, but %#`<fg:red>I am IMPORTANT RED text` and %#`<bold,underline,bg:#66FF00>I'm bolded and
/// underlined, with a bright green background` and %#`\<I'm not styled, as the < is escaped`
/// \endverbatim
/// <pre>I am a not formatted, but <span style="color:red">I am IMPORTANT RED text</span> and <span
/// style="background-color:#66FF00;font-weight:bold;text-decoration:underline">I'm bolded
/// and underlined, with a bright green background</span> and \<I'm not styled, as the \< is escaped</pre>
std::string render_blocks(std::string_view str, bool skip_styling);

/// Applies syntax highlighting to `str` by means of first passing it through inspection::Tokenizer,
/// then applying highlighting as specified by `opts` to the obtained tokens.
///
/// Special consideration is taken for literal blocks that should NOT be considered for tokenization
/// or syntax highlighting. These strings are surrounded by \`\` backticks and are removed before the tokenization
/// stage, then added back in the correct position afterwards. There are other properties that will be considered when
/// parsing this syntax that may be found at \ref parse_literal_strs.
std::string highlight(std::string_view str, const Options& opts = Options::get_default_options());

/// Returns a stringified and highlighted version of the supplied token.
/// Highlighting and spacing is done based on `opts`.
std::string highlight(const inspection::Token& token, const Options& opts = Options::get_default_options());

/// Returns a stringified and highlighted version of the supplied range of tokens.
/// Highlighting and spacing is done based on `opts`.
std::string highlight(std::span<const inspection::Token> tokens, const Options& opts = Options::get_default_options());

} // namespace asmgrader::highlight
