/// \file
/// Syntax highlighting of expressions as generated in \ref expression_inspection.hpp
/// using {fmt}'s color API.
#pragma once

#include <asmgrader/api/expression_inspection.hpp>

#include <fmt/base.h>
#include <fmt/color.h>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/primitives.hpp>

#include <array>
#include <cstddef>
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
        fmt::text_style style;
    };

    static constexpr std::array<Opt, num_token_kinds> default_token_options = []() {
        std::array<Opt, num_token_kinds> res{};

        constexpr auto idx = [](inspection::Token::Kind kind) { return static_cast<std::size_t>(kind); };

        res[idx(StringLiteral)] = Opt{fmt::fg(fmt::color::yellow)};
        res[idx(RawStringLiteral)] = Opt{fmt::fg(fmt::color::yellow)};
        res[idx(IntBinLiteral)] = res[idx(IntOctLiteral)] = res[idx(IntDecLiteral)] = res[idx(IntHexLiteral)] =
            Opt{fmt::fg(fmt::color::blue)};
        res[idx(FloatLiteral)] = res[idx(FloatLiteral)] = Opt{fmt::fg(fmt::color::blue)};

        return res;
    }();

    /// Each index is representative of the highlighting option for the token kind
    /// enumerator with a value == index.
    /// e.g., token_opts[0] is the option for StringLiteral
    std::array<Opt, num_token_kinds> token_opts = default_token_options;

    constexpr Opt& operator[](std::size_t idx) { return token_opts.at(idx); }

    constexpr Opt& operator[](inspection::Token::Kind kind) { return token_opts.at(static_cast<std::size_t>(kind)); }

    constexpr const Opt& operator[](std::size_t idx) const { return token_opts.at(idx); }

    constexpr const Opt& operator[](inspection::Token::Kind kind) const {
        return token_opts.at(static_cast<std::size_t>(kind));
    }
};

// If unspecified, uses the default of whatever text rendering method is in use

inline auto highlight(const inspection::Token& token, const Options& opts = {}) {
    return fmt::styled(token, opts[token.kind].style);
}

inline auto highlight(const ranges::sized_range auto& tokens, const Options& opts = {}) {
    std::vector<decltype(fmt::styled(std::declval<inspection::Token>(), std::declval<fmt::text_style>()))> result(
        ranges::size(tokens));

    ranges::transform(tokens, result.begin(), [&opts](const inspection::Token& tok) { return highlight(tok, opts); });

    return result;
}

} // namespace asmgrader::highlight
