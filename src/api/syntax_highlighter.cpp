#include "api/syntax_highlighter.hpp"

#include "api/expression_inspection.hpp"
#include "logging.hpp"

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/join.hpp>
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

std::string Options::Opt::apply(std::string_view str) const {
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

fmt::text_style Options::style_basic_ident_types(fmt::text_style type_style, fmt::text_style default_style,
                                                 std::string_view ident) {
    std::array basic_type_strs{"void", "int", "long", "unsigned", "float", "double", "char", "size_t"};

    if (ranges::contains(basic_type_strs, ident)) {
        return type_style;
    }

    return default_style;
}

fmt::text_style Options::style_op_keywords(fmt::text_style keyword_style, fmt::text_style default_style,
                                           std::string_view op) {
    ASSERT(!op.empty(), op);

    if (isalpha(op.at(0))) {
        return keyword_style;
    }

    return default_style;
}

std::string Options::basic_binary_op_spacing(std::string_view op) {
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

std::string Options::basic_op_spacing(std::string_view op) {
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

Options Options::get_default_options() {
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
    res[idx(FloatLiteral)] = res[idx(FloatLiteral)] = Opt{.style = fmt::fg(blue), .style_fn = {}, .transform_fn = {}};
    res[idx(Identifier)] = Opt{.style = {}, .style_fn = ident_style_fn, .transform_fn = {}};
    res[idx(Operator)] = Opt{.style = {}, .style_fn = op_style_fn, .transform_fn = basic_op_spacing};
    res[idx(BinaryOperator)] = Opt{.style = {}, .style_fn = op_style_fn, .transform_fn = basic_binary_op_spacing};

    return {res};
}

namespace {

/// POD for parsed result of a literal block
struct LiteralBlock
{
    /// Position of the first char of the start delimiter
    std::size_t start_delim{};
    /// Position of the end delimiter char
    std::size_t end_delim{};

    /// Start of the block, inclusive i.e. position after the starting delimiter and style spec
    std::size_t start{};
    /// End of the block, non-inclusive i.e. of the final \` delimiter
    /// Will be the same as end_delim for now
    std::size_t end{};

    /// Possibly empty sequence of style specifiers found between leading `<` `>` tokens
    /// E.g.:
    /// \verbatim
    /// %#`<bold,fg:#FFFFFF,bg:blue>this is text`
    /// \endverbatim
    /// style_specs = `{"bold", "fg:#FFFFFF", "bg:blue"}`
    std::vector<std::string_view> style_specs;
};

/// Parse a style spec and convert it to the corresponding fmt style
///
/// Invalid styles are parsed as the default style and cause a warning to be emmitted.
fmt::text_style spec_to_fmt_style(std::string_view spec) noexcept {
    // for logging purposes
    std::string_view orig_spec = spec;

    using EV = std::pair<std::string_view, fmt::text_style>;

    // `bold` | `italic` | `underline` | `blink` | `strikethrough`
    static const std::array emphases_map{
        EV{"bold", fmt::emphasis::bold},
        EV{"italic", fmt::emphasis::italic},
        EV{"underline", fmt::emphasis::underline},
        EV{"blink", fmt::emphasis::blink},
        EV{"strikethrough", fmt::emphasis::strikethrough},
    };

    using CV = std::pair<std::string_view, fmt::terminal_color>;
    // black, red, green, yellow, blue, magenta, cyan, or white
    static const std::array ansi_colors_map{
        CV{"black", fmt::terminal_color::black}, CV{"red", fmt::terminal_color::red},
        CV{"green", fmt::terminal_color::green}, CV{"yellow", fmt::terminal_color::yellow},
        CV{"blue", fmt::terminal_color::blue},   CV{"magenta", fmt::terminal_color::magenta},
        CV{"cyan", fmt::terminal_color::cyan},   CV{"white", fmt::terminal_color::white},
    };

    constexpr auto key = [](const auto& pair) { return pair.first; };

    if (const auto* iter = ranges::find(emphases_map, spec, key); iter != ranges::end(emphases_map)) {
        return iter->second;
    }

    // either fg or bg
    std::function<fmt::text_style(fmt::detail::color_type)> color_fn;

    if (spec.starts_with("fg:")) {
        color_fn = fmt::fg;
    } else if (spec.starts_with("bg:")) {
        color_fn = fmt::bg;
    } else {
        LOG_WARN("unknown block literal style spec: {:?}", orig_spec);
        return {};
    }

    // remove "fg:" / "bg:"
    spec.remove_prefix(3);
    if (spec.empty()) {
        LOG_WARN("bad block literal style spec: no color after fg:/bg: {:?}", orig_spec);
        return {};
    }

    if (spec.starts_with('#')) {
        spec.remove_prefix(1);

        if (spec.size() != 6) {
            LOG_WARN("bad block literal style spec: hex color is too short (should be 6 chars) {:?}", orig_spec);
            return {};
        }

        fmt::underlying_t<fmt::color> int_res{};
        auto from_chars_res = std::from_chars(&spec.front(), &spec.back() + 1, int_res, /*base=*/16); // NOLINT
        if (from_chars_res.ec != std::errc{}) {
            LOG_WARN("bad block literal style spec: could not parse hex color (errc={}) {:?}",
                     fmt::underlying(from_chars_res.ec), orig_spec);
            return {};
        }

        fmt::color fmt_color{int_res};
        return color_fn(fmt_color);
    }

    if (const auto* iter = ranges::find(ansi_colors_map, spec, key); iter != ranges::end(ansi_colors_map)) {
        return color_fn(iter->second);
    }

    LOG_WARN("bad block literal style spec: could not parse fg:/bg: color {:?}", orig_spec);
    return {};
}

// Call with the style spec BETWEEN `<` `>` delimiters (non-inclusive)
std::vector<std::string_view> parse_style_specs(std::string_view str) noexcept {
    std::vector<std::string_view> results;

    // ranges::views::split annoyingly does not provide a contiguous iterator...
    for (std::size_t start = 0; const auto& [i, chr] : str | ranges::views::enumerate) {
        // If we've reached a , or we're at the end of the stream, parse the spec token
        if (chr == ',') {
            results.push_back(str.substr(start, i - start));
            start = i + 1;
        } else if (i + 1 == str.size()) {
            results.push_back(str.substr(start));
        }
    }

    return results;
}

/// Find and parse all literal blocks in str
///
/// Any invalid style specs, missing <code>`</code>, or other erroneous formatting is simply skipped
/// and may appear in the output. A warning is emmitted via \ref LOG_WARN if this happens.
///
/// The return value of this function must not outlive the string pointed to by `str`
std::vector<LiteralBlock> find_literal_blocks(std::string_view str) noexcept {
    std::vector<LiteralBlock> results;

    for (std::size_t idx = 0, start_delim = str.find("%#`"); start_delim != std::string_view::npos;
         start_delim = str.find("%#`")) {
        // create a new block structure
        results.emplace_back();
        auto& current_block = results.back();

        idx += start_delim;
        current_block.start_delim = idx;
        // remove full start delimiter
        str.remove_prefix(start_delim + 3);
        // We want to start one past the final char of the delim
        idx += 3;

        if (str.starts_with('<')) {
            std::size_t end_style_spec = str.find('>');

            // if the ending `>` is not present OR there is any whitespace between `<` `>` then don't consider it as a
            // style spec
            if (auto substr = str.substr(1, end_style_spec - 1);
                substr.empty() || end_style_spec == std::string_view::npos ||
                ranges::find_if(substr, isspace) != ranges::end(substr)) {
                LOG_WARN("literal block style spec is invalid. start-idx={}, substr={:?}, str={:?}", idx, substr, str);
            } else {
                current_block.style_specs = parse_style_specs(substr);
                idx += end_style_spec + 1;
                str.remove_prefix(end_style_spec + 1); // remove through '>' delim
            }
        } else if (str.starts_with("\\<")) {
            // include the literal < char if it's escaped
            idx++;
        }
        current_block.start = idx;

        // keep searching for an end delim ` while the one we found is not escaped by a backslash
        std::size_t end_delim{};
        while (true) {
            end_delim = str.find('`');
            if (end_delim == std::string_view::npos) {
                break;
            }

            // place idx at position of the (potential) ` delim
            idx += end_delim;

            bool done = end_delim == 0 || str.at(end_delim - 1) != '\\';

            str.remove_prefix(end_delim + 1); // remove through the ` delim

            if (done) {
                break;
            }

            // move the idx up to skip the \ as well
            idx++;
        }

        // missing end delimiter -> just extend to the end of the string
        // This is probably the sanest option when considering this is used to disable syntax highlighting
        // some random text being syntax highlighted like c++ would look very strange.
        if (end_delim == std::string_view::npos) {
            LOG_WARN("literal block is invalid because there's no ending ` delim. start-idx={}, str={:?}", idx, str);
            current_block.end = static_cast<std::size_t>(-1);
            current_block.end_delim = static_cast<std::size_t>(-1);
            // we never found a `, so we should never be able to find a %#` to loop again, but break anyways just to be
            // safe
            break;
        }

        current_block.end = idx;
        current_block.end_delim = idx;
        // move idx past the end ` delim
        idx++;
    }

    return results;
}

std::string render_literal_block(std::string_view source, const LiteralBlock& block, bool skip_styling) {
    std::string res_str;
    res_str.reserve(block.end - block.start);

    // remove \ from escaped backticks
    // \` => `
    for (char prev{}; char c : source.substr(block.start, block.end - block.start)) {
        if (prev == '\\' && c == '`') {
            // Remove the last \ char
            res_str.erase(res_str.end() - 1);
        }

        res_str += c;
        prev = c;
    }

    if (skip_styling) {
        return res_str;
    }

    fmt::text_style combined_styles = ranges::fold_left(block.style_specs | ranges::views::transform(spec_to_fmt_style),
                                                        fmt::text_style{}, std::bit_or{});

    return fmt::to_string(fmt::styled(res_str, combined_styles));
}

constexpr std::string_view block_sentinel{"\0", 1};

/// Extract the substrings that are not part of literal blocks.
///
/// A string of "\0" is used as a sentinal to denote that it should be replaced by a rendered block
///
/// The data pointed to by str must outlive the return value of this function.
std::vector<std::string_view> split_non_literals(std::string_view str, const std::vector<LiteralBlock>& blocks) {
    std::vector<std::string_view> results;
    std::size_t start = 0;
    for (const LiteralBlock& block : blocks) {
        if (block.start_delim == start) {
            results.push_back(block_sentinel);
            start = block.end_delim + 1;
            continue;
        }

        std::string_view substr = str.substr(start, block.start_delim - start);
        start = block.end_delim + 1;

        results.push_back(substr);
        results.push_back(block_sentinel);
    }

    if (start < str.size()) {
        results.push_back(str.substr(start));
    }

    return results;
}

/// Merges a potentially empty list of literal blocks into a vector. Accepts a range with non-literal strings and
/// sentinals indicating a placeholder position for a literal
///
/// \returns A range of `std::string`s
ranges::range auto merge_literal_blocks(std::string_view str, const std::vector<LiteralBlock>& blocks,
                                        const ranges::forward_range auto& strs_rng, bool skip_styling) {
    auto tf_fun = [block_iter = blocks.begin(), &blocks, str, skip_styling](std::string_view component) mutable {
        if (component == block_sentinel) {
            if (block_iter == blocks.end()) {
                LOG_WARN("literal block rendering error: too few literal block objects. str={:?}, "
                         "num-blocks={}",
                         str, blocks.size());
            } else {
                return render_literal_block(str, *block_iter++, skip_styling);
            }
        }
        return std::string{component};
    };

    return strs_rng | ranges::views::transform(tf_fun);
}

} // namespace

std::string render_blocks(std::string_view str, bool skip_styling) {
    std::vector<LiteralBlock> blocks = find_literal_blocks(str);
    std::vector<std::string_view> split_strs = split_non_literals(str, blocks);

    auto all_components = merge_literal_blocks(str, blocks, split_strs, skip_styling);

    return all_components | ranges::views::join | ranges::to<std::string>();
}

std::string highlight(std::string_view str, const Options& opts) {
    // highlight the string if it's not equal to the literal block sentinal
    auto potential_highlight = [&opts](const std::string_view component) -> std::string {
        if (component == block_sentinel) {
            return std::string{component};
        }

        return highlight(component, opts);
        //               ^^^ implicit conversion to inspection::Token
    };

    std::vector<LiteralBlock> blocks = find_literal_blocks(str);
    std::vector<std::string_view> split_strs = split_non_literals(str, blocks);

    auto syntax_highlighted_strs = split_strs | ranges::views::transform(potential_highlight);

    auto all_components = merge_literal_blocks(str, blocks, syntax_highlighted_strs, /*skip_styling=*/false);

    return all_components | ranges::views::join | ranges::to<std::string>();
}

std::string highlight(const inspection::Token& token, const Options& opts) {
    return opts[token.kind].apply(token.str);
}

std::string highlight(std::span<const inspection::Token> tokens, const Options& opts) {
    return tokens | ranges::views::transform([&opts](const inspection::Token& tok) { return highlight(tok, opts); }) |
           ranges::views::join | ranges::to<std::string>();
}

} // namespace asmgrader::highlight
