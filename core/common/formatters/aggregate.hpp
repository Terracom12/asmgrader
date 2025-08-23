#pragma once

#include "common/formatters/generic_impl.hpp"

#include <boost/describe/enumerators.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/range/concepts.hpp>

#include <string>
#include <string_view>
#include <type_traits>

namespace util::detail {

template <typename Aggregate>
    requires(std::is_aggregate_v<Aggregate> && std::is_standard_layout_v<Aggregate> && !std::is_array_v<Aggregate> &&
             !ranges::range<Aggregate>)
struct FormatterImpl<Aggregate>
{
    static auto format(const Aggregate& from, fmt::format_context& ctx) {
        // if (is_debug_format) {
        //     return debug_format(from, ctx);
        // }
        return normal_format(from, ctx);
    }

    static auto normal_format(const Aggregate& from, fmt::format_context& ctx) {
        ctx.advance_to(fmt::format_to(ctx.out(), "{}", boost::typeindex::type_id<Aggregate>().pretty_name()));

        *ctx.out()++ = '{';

        boost::pfr::for_each_field_with_name(from,
                                             [&, first = true](std::string_view field_name, const auto& val) mutable {
                                                 if (!first) {
                                                     ctx.advance_to(fmt::format_to(ctx.out(), ", "));
                                                 }
                                                 first = false;
                                                 ctx.advance_to(fmt::format_to(ctx.out(), ".{}={}", field_name, val));
                                             });
        *ctx.out()++ = '}';

        return ctx.out();
    }

    static auto debug_format(const Aggregate& from, fmt::format_context& ctx) {
        static std::string indent;

        ctx.advance_to(
            fmt::format_to(ctx.out(), "{}{} {{\n", indent, boost::typeindex::type_id<Aggregate>().pretty_name()));

        indent += '\t';

        boost::pfr::for_each_field_with_name(
            from, [&, first = true](std::string_view field_name, const auto& val) mutable {
                if (!first) {
                    ctx.advance_to(fmt::format_to(ctx.out(), ",\n"));
                }
                first = false;
                ctx.advance_to(fmt::format_to(ctx.out(), "{}.{} = {}", indent, field_name, val));
            });

        indent.pop_back();

        ctx.advance_to(fmt::format_to(ctx.out(), "\n{}}}", indent));

        return ctx.out();
    }
};

} // namespace util::detail
