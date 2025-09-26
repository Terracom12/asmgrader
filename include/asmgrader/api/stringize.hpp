/// \file
/// See \ref asmgrader::stringize
#pragma once

#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/formatters/unknown.hpp>
#include <asmgrader/logging.hpp>

#include <boost/type_index.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/transform.hpp>

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <utility>

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

/// Result type of a call to `str` or `repr`
struct StringizeResult
{
    std::string resolve_blocks(bool do_colorize) const;
    std::string syntax_highlight() const;

    std::string original;
};

namespace detail {

// these are defined up here to allow for nested calls in str_default

struct ReprFn
{
    template <typename T>
    StringizeResult operator()(std::span<const inspection::Token> tokens, std::string_view raw_str, const T& val) const;

    template <typename T>
    constexpr fmt::formattable auto pre(std::span<const inspection::Token> tokens, std::string_view raw_str,
                                        const T& val) const;
};

constexpr ReprFn repr_fn_dispatcher{};

/// `str` function dispatcher
struct StrFn
{
    template <typename T>
    StringizeResult operator()(const T& val) const;

    template <typename T>
    constexpr fmt::formattable auto pre(const T& val) const;
};

constexpr StrFn str_fn_dispatcher{};

inline std::string str_default(const std::convertible_to<std::string_view> auto& val) {
    return fmt::format("{:?}", std::string_view{val});
}

// TODO: base output shennanigans
inline std::string str_default(std::integral auto val) {
    return fmt::format("{}", val);
}

inline std::string str_default(char val) {
    return fmt::format("{:?}", val);
}

// TODO: binary / hex output shennanigans
inline std::string str_default(std::floating_point auto val) {
    return fmt::format("{}", val);
}

template <ranges::range Range>
    requires(!std::convertible_to<Range, std::string_view>) // prevent ambiguity with other overload
inline std::string str_default(const Range& val) {
    return fmt::to_string(fmt::join(
        val | ranges::views::transform([](const auto& elem) { return str_fn_dispatcher(elem).original; }), ", "));
}

template <typename T, typename E>
inline std::string str_default(const Expected<T, E>& expected) {
    // TODO: some kind of (globally?) configurable color scheme, use an "error color" here
    if (expected.has_value()) {
        if constexpr (std::same_as<T, void>) {
            return "void";
        } else {
            // FIXME: ??
            // hmm... Still not a template expert. I hope this doesn't instantiate and resolve overloads here.
            return str_fn_dispatcher(expected.value()).original;
        }
    }
    std::string err_str = [&expected]() -> std::string {
        if constexpr (std::same_as<E, void>) {
            return "void";
        } else {
            // don't want to nest within the error coloring block below, since that's still not supported
            return str_fn_dispatcher(expected.error()).resolve_blocks(/*do_colorize=*/false);
        }
    }();
    return fmt::format("%#`<fg:red>{}`", err_str);
}

inline std::string str_default(ErrorKind error_kind) {
    return fmt::format("{}", error_kind);
}

template <typename T>
StringizeResult ReprFn::operator()(std::span<const inspection::Token> tokens, std::string_view raw_str,
                                   const T& val) const {
    return {fmt::to_string(pre(tokens, raw_str, val))};
}

template <typename T>
constexpr fmt::formattable auto ReprFn::pre(std::span<const inspection::Token> tokens, std::string_view raw_str,
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

/// `str` function dispatcher
template <typename T>
StringizeResult StrFn::operator()(const T& val) const {
    return {fmt::to_string(pre(val))};
}

template <typename T>
constexpr fmt::formattable auto StrFn::pre(const T& val) const {
    if constexpr (requires { val.str(); }) {
        return val.str(); // member function
    } else if constexpr (requires { str(val); }) {
        return str(val); // free function found via ADL
    } else if constexpr (requires { str_default(val); }) {
        return str_default(val); // default defined BEFORE this struct
    } else {
        LOG_WARN("No `str` implementation for type <{}>", boost::typeindex::type_id<T>().pretty_name());
        return "%#`...`"; // fallback
    }
}

} // namespace detail

/// `repr` customization point object
/// The return value of this function is guaranteed to be parsable by \ref highlight::highlight
///
/// Users may define a free function `repr` with the following signature:
///   fmt::formattable auto repr(std::span<const Token> tokens, std::string_view raw_str, const T& val)
/// Where T is any user-defined type. This function must be defined in the namespace enclosing `T`.
///
/// `repr` may also be defined within a user-defined type as a non-static member function. This function's required
/// signature is:
///   fmt::formattable auto repr(std::span<const Token> tokens, std::string_view raw_str) const
inline constexpr const auto& repr = detail::repr_fn_dispatcher;

/// `str` customization point object
/// The return value of this function is guaranteed to be parsable by \ref highlight::highlight
///
/// Users may define a free function `str` with the following signature:
///   fmt::formattable auto str(const T& val)
/// Where T is any user-defined type. This function must be defined in the namespace enclosing `T`.
///
/// `str` may also be defined within a user-defined type as a non-static member function. This function's required
/// signature is:
///   fmt::formattable auto str() const
inline constexpr const auto& str = detail::str_fn_dispatcher;

} // namespace asmgrader::stringize
