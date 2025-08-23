#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

struct DebugFormatter
{
    bool is_debug_format = false;

    constexpr auto parse(fmt::format_parse_context& ctx) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        if (it != end && *it == '?') {
            is_debug_format = true;
            ++it;
        } else if (it != end && *it != '}') {
            throw fmt::format_error("invalid format");
        }

        return it;
    }
};
