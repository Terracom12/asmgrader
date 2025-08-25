#pragma once

#include <asmgrader/common/formatters/debug.hpp>

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
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <variant>

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
struct fmt::formatter<std::error_code> : ::asmgrader::DebugFormatter
{
    auto format(const std::error_code& from, format_context& ctx) const {
        return format_to(ctx.out(), "{} : {}", strerrorname_np(from.value()), from.message());
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>> : ::asmgrader::DebugFormatter
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
struct fmt::formatter<std::variant<Ts...>> : ::asmgrader::DebugFormatter
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
