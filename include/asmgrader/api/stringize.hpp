/// \file
/// See \ref asmgrader::stringize
#pragma once

#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/common/formatters/unknown.hpp>
#include <asmgrader/logging.hpp>

#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#include <concepts>
#include <span>
#include <string>
#include <string_view>

/// For everything related to test requirement stringification
///
/// Implementations of the `str` and `repr` functions.
/// These by inspired by Python functions of the same names, and conform to basically the same semantics.
/// That is,
///     `repr` returns the "raw" *representation* of an expression.
///     This is what should be used when we want to display components of an expression statement.
///
///     `str` returns the nicely-formatted *value* of an expression.
///     This is what should be used when we want to display the evalutions of subexpressions.
///
/// Output of will default to \ref format_or_unknown for all types without a stringize overload.
namespace asmgrader::stringize {

namespace detail {

std::string str_default(const std::convertible_to<std::string_view> auto& val) {
    return fmt::format("{:?}", std::string_view{val});
}

// TODO: base output shennanigans
std::string str_default(std::integral auto val) {
    return fmt::format("{}", val);
}

// TODO: binary / hex output shennanigans
std::string str_default(std::floating_point auto val) {
    return fmt::format("{}", val);
}

/// `repr` function dispatcher
struct ReprFn
{
    template <typename T>
    std::string operator()(std::span<const inspection::Token> tokens, std::string_view raw_str, const T& val) const {
        return fmt::to_string(pre(tokens, raw_str, val));
    }

    template <typename T>
    constexpr fmt::formattable auto pre(std::span<const inspection::Token> tokens, std::string_view raw_str,
                                        const T& val) const {
        if constexpr (requires { val.repr(tokens, raw_str); }) {
            return val.repr(tokens, raw_str); // member function
        } else if constexpr (requires { repr(tokens, raw_str, val); }) {
            return repr(tokens, raw_str, val); // free function found via ADL
        } else if constexpr (requires { repr_default(tokens, raw_str, val); }) {
            return repr_default(tokens, raw_str, val); // default defined BEFORE this struct
        } else {
            return raw_str; // fallback
        }
    }
};

/// `str` function dispatcher
struct StrFn
{
    template <typename T>
    std::string operator()(const T& val) const {
        return fmt::to_string(pre(val));
    }

    template <typename T>
    constexpr fmt::formattable auto pre(const T& val) const {
        if constexpr (requires { val.str(); }) {
            return val.str(); // member function
        } else if constexpr (requires { str(val); }) {
            return str(val); // free function found via ADL
        } else if constexpr (requires { str_default(val); }) {
            return str_default(val); // default defined BEFORE this struct
        } else {
            LOG_WARN("No `str` implementation for type <{}>", boost::typeindex::type_id<T>().pretty_name());
            return "<unknown>"; // fallback
        }
    }
};

} // namespace detail

/// `repr` customization point object
/// The return value of this function is guaranteed to be parsable by \ref inspection::Tokenizer
///
/// Users may define a free function `repr` with the following signature:
///   fmt::formattable auto repr(std::span<const Token> tokens, std::string_view raw_str, const T& val)
/// Where T is any user-defined type. This function must be defined in the namespace enclosing `T`.
///
/// `repr` may also be defined within a user-defined type as a non-static member function. This function's required
/// signature is:
///   fmt::formattable auto repr(std::span<const Token> tokens, std::string_view raw_str) const
constexpr detail::ReprFn repr{};

/// `str` customization point object
/// The return value of this function is guaranteed to be parsable by \ref inspection::Tokenizer
///
/// Users may define a free function `str` with the following signature:
///   fmt::formattable auto str(const T& val)
/// Where T is any user-defined type. This function must be defined in the namespace enclosing `T`.
///
/// `str` may also be defined within a user-defined type as a non-static member function. This function's required
/// signature is:
///   fmt::formattable auto str() const
constexpr detail::StrFn str{};

} // namespace asmgrader::stringize
