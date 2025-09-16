#pragma once

#include <asmgrader/common/formatters/debug.hpp>
#include <asmgrader/common/formatters/fmt_appender_adaptor.hpp>
#include <asmgrader/common/formatters/generic_impl.hpp>
#include <asmgrader/common/static_string.hpp>

#include <boost/describe/enum.hpp>
#include <boost/describe/enumerators.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/range/concepts.hpp>

#include <optional>
#include <string_view>
#include <utility>

namespace asmgrader::detail {

/// \tparam Fields - std::pair<StaticString, Enum>
///                            field name    field value
///                  Very annoying to declare manually, so use the macro instead!
template <typename Enum, StaticString EnumName, auto... Enumerators>
struct EnumFormatter
{
    DebugFormatter debug_parser;

    constexpr auto parse(fmt::format_parse_context& ctx) {
        // parse a potential '?' spec
        return debug_parser.parse(ctx);
    }

    constexpr auto get_enumerator(const Enum& from) const {
        auto val = [from] {
            std::optional<std::pair<std::string_view, Enum>> res;
            ((Enumerators.second == from ? res = Enumerators : res), ...);
            return res;
        }();

        return val;
    }

    constexpr auto normal_format(const Enum& from, fmt::format_context& ctx) const {
        auto val = get_enumerator(from);

        if (val.has_value()) {
            return fmt::format_to(ctx.out(), "{}", val->first);
        }

        return fmt::format_to(ctx.out(), "<unknown ({})>", fmt::underlying(from));
    }

    constexpr auto debug_format(const Enum& from, fmt::format_context& ctx) const {
        fmt_appender_wrapper ctx_iter{ctx.out()};

        fmt::format_to(ctx_iter, "{}{{", EnumName);

        ctx.advance_to(normal_format(from, ctx));

        ranges::copy("}", ctx_iter);

        return ctx_iter.out;
    }

    constexpr auto format(const Enum& from, fmt::format_context& ctx) const {
        if (debug_parser.is_debug_format) {
            return debug_format(from, ctx);
        }
        // else
        return normal_format(from, ctx);
    }
};

} // namespace asmgrader::detail
