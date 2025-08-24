#pragma once

#include "common/formatters/generic_impl.hpp"

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

namespace ct_test {

enum class SEa {};
enum class SEb { A, B };
enum class SEc {};
enum class SEd { A, B };

enum Ea {};

enum Eb { A, B };

enum Ec {};

enum Ed { C, D };

// NOLINTBEGIN
BOOST_DESCRIBE_ENUM(SEa);
BOOST_DESCRIBE_ENUM(SEb, A, B);
BOOST_DESCRIBE_ENUM(Ea);
BOOST_DESCRIBE_ENUM(Eb, A, B);
// NOLINTEND

static_assert(fmt::formattable<SEa>);
static_assert(fmt::formattable<SEb>);
static_assert(!fmt::formattable<SEc>);
static_assert(!fmt::formattable<SEd>);

static_assert(fmt::formattable<Ea>);
static_assert(fmt::formattable<Eb>);
static_assert(!fmt::formattable<Ec>);
static_assert(!fmt::formattable<Ed>);

} // namespace ct_test
