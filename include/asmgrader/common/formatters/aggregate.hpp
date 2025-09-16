#pragma once

#include <asmgrader/common/formatters/debug.hpp>
#include <asmgrader/common/formatters/fmt_appender_adaptor.hpp>
#include <asmgrader/common/formatters/generic_impl.hpp>
#include <asmgrader/common/static_string.hpp>

#include <boost/describe/enumerators.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/pfr.hpp>
#include <boost/pfr/traits.hpp>
#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/range/concepts.hpp>

#include <string_view>
#include <tuple>
#include <utility>

namespace asmgrader::detail {

/// \tparam Fields - std::pair<StaticString, Aggregate::*>
///                            field name    field ptr
///                  Very annoying to declare manually, so use the macro below!
template <typename Aggregate, StaticString AggregateName, auto... Fields>
struct AggregateFormatter
{
    DebugFormatter debug_parser;

    constexpr auto parse(fmt::format_parse_context& ctx) {
        // parse a potential '?' spec
        return debug_parser.parse(ctx);
    }

    constexpr auto get_fields(const Aggregate& from) const { return std::tuple{((&from)->*(Fields.second))...}; }

    constexpr auto get_named_fields(const Aggregate& from) const {
        return std::tuple{std::pair{Fields.first, ((&from)->*(Fields.second))}...};
    }

    constexpr auto normal_format(const Aggregate& from, fmt::format_context& ctx) const {
        // Seperator between elements
        constexpr std::string_view sep = ", ";

        const std::tuple fields{get_fields(from)};

        return fmt::format_to(ctx.out(), "{}", fmt::join(fields, sep));
    }

    constexpr auto debug_format(const Aggregate& from, fmt::format_context& ctx) const {
        fmt_appender_wrapper ctx_iter{ctx.out()};

        // Seperator between elements
        constexpr std::string_view sep = ", ";
        // Leading and trailing output
        constexpr std::string_view leading = " {";
        constexpr std::string_view trailing = "}";

        const std::tuple fields{get_named_fields(from)};

        // Transform fields to printable output`
        auto field_writer = [&, this, first = true](const auto& pair) mutable {
            if (!first) {
                ranges::copy(sep, ctx_iter);
            }
            first = false;

            fmt::format_to(ctx_iter.out, ".{} = {}", pair.first, pair.second);
        };

        fmt::format_to(ctx_iter, "{}{}", AggregateName, leading);

        std::apply([&field_writer](const auto&... elems) { (field_writer(elems), ...); }, fields);

        ranges::copy(trailing, ctx_iter);

        return ctx_iter.out;
    }

    constexpr auto format(const Aggregate& from, fmt::format_context& ctx) const {
        if (debug_parser.is_debug_format) {
            return debug_format(from, ctx);
        }
        // else
        return normal_format(from, ctx);
    }
};

} // namespace asmgrader::detail
