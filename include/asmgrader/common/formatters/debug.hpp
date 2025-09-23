#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

namespace asmgrader {

struct DebugFormatter
{
    bool is_debug_format = false;

    constexpr auto parse(fmt::format_parse_context& ctx) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        if (it != end && *it == '?') {
            is_debug_format = true;
            ++it;
        }

        return it;
    }
};

} // namespace asmgrader
