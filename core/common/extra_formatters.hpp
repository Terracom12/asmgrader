#pragma once

#include <boost/describe.hpp>
#include <boost/describe/enum.hpp>
#include <boost/describe/enumerators.hpp>
#include <boost/mp11.hpp>
#include <boost/pfr.hpp>
#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/range/concepts.hpp>

#include <concepts>
#include <ctime>
#include <exception>
#include <filesystem>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <string.h>

namespace util {

template <typename T>
inline std::string fmt_or_unknown(T&& value, fmt::fstring<T> fmt = "{}") {
    if constexpr (fmt::formattable<T>) {
        return fmt::format(fmt, std::forward<T>(value));
    } else {
        return "<unknown>";
    }
}

} // namespace util

struct DebugFormatter
{
    bool is_debug_format = false;

    constexpr auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && *it == '?') {
            is_debug_format = true;
        } else if (it != end && *it != '}') {
            throw fmt::format_error("invalid format");
        }

        while (it != end && *it != '}') {
            ++it;
        }

        return it;
    }
};

template <>
struct fmt::formatter<std::exception> : formatter<std::string>
{
    auto format(const std::exception& from, fmt::format_context& ctx) const {
        std::string str = fmt::format("{}: '{}'", boost::typeindex::type_id_runtime(from).pretty_name(), from.what());

        return formatter<std::string>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<std::filesystem::path> : formatter<std::string>
{
    auto format(const std::filesystem::path& from, fmt::format_context& ctx) const {
        return formatter<std::string>::format(from.string(), ctx);
    }
};

template <>
struct fmt::formatter<std::timespec> : formatter<std::string>
{
    auto format(const std::timespec& from, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "timespec{{tv_sec={}, tv_nsec={}}}", from.tv_sec, from.tv_nsec);
    }
};

template <>
struct fmt::formatter<std::source_location> : formatter<std::string>
{
    auto format(const std::source_location& from, fmt::format_context& ctx) const {
        return formatter<std::string>::format(
            fmt::format("[{}@{}:{}:{}]", from.file_name(), from.line(), from.column(), from.function_name()), ctx);
    }
};

// /// Formatter for std::timespec
// inline std::string format_as(const timespec& from) {
//     return fmt::format("timespec{{tv_sec={}, tv_nsec={}}}", from.tv_sec, from.tv_nsec);
// }

/// Output formatter for make_error_code
template <>
struct fmt::formatter<std::error_code> : DebugFormatter
{
    auto format(const std::error_code& from, format_context& ctx) const {
        return format_to(ctx.out(), "{} : {}", strerrorname_np(from.value()), from.message());
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>> : DebugFormatter
{
    auto format(const std::optional<T>& from, format_context& ctx) const {
        if (!from) {
            return fmt::format_to(ctx.out(), "nullopt");
        }

        if (is_debug_format) {
            return fmt::format_to(ctx.out(), "Optional({})", from.value());
        }

        return fmt::format_to(ctx.out(), "{}", from.value());
    }
};

template <typename... Ts>
struct fmt::formatter<std::variant<Ts...>> : DebugFormatter
{
    auto format(const std::variant<Ts...>& from, format_context& ctx) const {
        const auto& [type_str, inner_str] = std::visit(
            [](auto&& val) {
                std::string inner;
                if constexpr (std::convertible_to<decltype(val), std::string_view>) {
                    inner = fmt::format("{:?}", val);
                } else {
                    inner = fmt::format("{}", val);
                }
                return std::tuple{boost::typeindex::type_id<decltype(val)>().pretty_name(), inner};
            },
            from);

        if (is_debug_format) {
            return fmt::format_to(ctx.out(), "Variant<T={}>({})", type_str, inner_str);
        }

        return fmt::format_to(ctx.out(), "{}", inner_str);
    }
};

namespace util {

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

template <typename T, typename Enable = void>
struct FormatterImpl;

template <typename Enum>
    requires requires(Enum scoped_enum) { enum_to_string(scoped_enum); }
struct FormatterImpl<Enum>
{
    static auto format(const Enum& from, fmt::format_context& ctx) {
        auto res = util::enum_to_string(from);

        if (res) {
            return fmt::format_to(ctx.out(), "{}", *res);
        }

        return fmt::format_to(ctx.out(), "<unknown ({})>", fmt::underlying(from));
    }
};

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

}; // namespace util

/// Formatter for enum classes and aggregates
template <typename T>
    requires requires { util::FormatterImpl<T>(); }
struct fmt::formatter<T> : DebugFormatter
{
    constexpr auto format(const T& from, fmt::format_context& ctx) const {
        return util::FormatterImpl<T>::format(from, ctx);
    }
};

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
