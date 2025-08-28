#pragma once

#include <asmgrader/common/formatters/generic_impl.hpp>

#include <boost/describe/enum.hpp>
#include <boost/describe/enumerators.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/range/concepts.hpp>

#include <optional>
#include <type_traits>

namespace asmgrader::detail {

// See: https://www.boost.org/doc/libs/1_81_0/libs/describe/doc/html/describe.html#example_printing_enums_ct
template <typename Enum>
    requires(std::is_enum_v<Enum> && boost::describe::has_describe_enumerators<Enum>::value)
constexpr std::optional<const char*> enum_to_string(Enum enumerator) {
    std::optional<const char*> res;

    boost::mp11::mp_for_each<boost::describe::describe_enumerators<Enum>>([&](auto descriptor) {
        if (enumerator == descriptor.value) {
            res = descriptor.name;
        }
    });

    return res;
}

template <typename Enum>
    requires requires(Enum scoped_enum) { enum_to_string(scoped_enum); }
struct FormatterImpl<Enum>
{
    static auto format(const Enum& from, fmt::format_context& ctx) {
        auto res = enum_to_string(from);

        if (res) {
            return fmt::format_to(ctx.out(), "{}", *res);
        }

        return fmt::format_to(ctx.out(), "<unknown ({})>", fmt::underlying(from));
    }
};

} // namespace asmgrader::detail
