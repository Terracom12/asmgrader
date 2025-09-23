#pragma once

#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/api/stringize.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/to_static_range.hpp>
#include <asmgrader/logging.hpp>
#include <asmgrader/meta/always_false.hpp>
#include <asmgrader/program/program.hpp>
#include <asmgrader/subprocess/memory/concepts.hpp>

#include <fmt/base.h>
#include <fmt/format.h>
#include <gsl/narrow>
#include <gsl/util>
#include <libassert/assert.hpp>
#include <range/v3/action/split.hpp>
#include <range/v3/view/enumerate.hpp>

#include <array>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace asmgrader {

/// Transparent wrapper around Result<Ret>, as far as the user is concerned.
///
/// Internally provides serialized_args and function_name member variables for
/// use in requirement serialization.
template <typename Ret, typename... Args>
class AsmFunctionResult : public Result<Ret>
{
public:
    // NOLINTNEXTLINE(readability-identifier-naming) - rational: don't shadow member variables
    constexpr AsmFunctionResult(std::tuple<std::decay_t<Args>...> args_, std::string_view function_name_)
        : args{std::move(args_)}
        , function_name{function_name_} {}

    template <typename U>
    void set_result(U&& val) {
        Result<Ret>::operator=(std::forward<U>(val));
    }

    std::tuple<std::decay_t<Args>...> args;
    std::string_view function_name;

    std::string repr(std::span<const inspection::Token> tokens, std::string_view raw_str) const {
        // auto split_arg_tokens_fn = [open_groupings = std::stack<char>{}](const inspection::Token& tok) mutable {
        //     using inspection::Token::Kind::Grouping;
        //
        //     // Opening grouping symbol
        //     if (tok.str == "(" || tok.str == "{" || (tok.kind == Grouping && tok.str == "<")) {
        //         open_groupings.push(tok.str.at(0));
        //     }
        //     // Closing grouping symbol
        //     else if (!open_groupings.empty() &&
        //              (tok.str == ")" || tok.str == "}" || (tok.kind == Grouping && tok.str == ">"))) {
        //         // This *should* always be true, but it's not important enough to crash in a release build
        //         DEBUG_ASSERT(open_groupings.top() ==
        //                      (std::map<char, char>{{')', '('}, {'}', '{'}, {'>', '<'}}).at(tok.str.at(0)));
        //         open_groupings.pop();
        //     }
        //
        //     return open_groupings.empty() && tok.str == ",";
        // };

        // FIXME: This is very much hacked together and will likely break in a lot of edge cases

        // std::array<std::span<const inspection::Token>, sizeof...(Args)> arg_tokens{};
        // std::array<std::string_view, sizeof...(Args)> arg_raw_strs{};

        // DEBUG_ASSERT(tokens[0].kind == inspection::Token::Kind::Identifier);
        // DEBUG_ASSERT(tokens[1] == (inspection::Token{.kind = inspection::Token::Kind::Operator, .str = "("}),
        //              tokens[1].kind, tokens[1].str);
        // DEBUG_ASSERT(tokens.back() == (inspection::Token{.kind = inspection::Token::Kind::Operator, .str = ")"}),
        //              tokens.back().kind, tokens.back().str);
        //
        // tokens = tokens.subspan(2);
        //
        // for (std::size_t last_start = 0, len = 0, i = 0; const auto& [ti, tok] : tokens | ranges::views::enumerate) {
        //     if (i >= arg_tokens.size()) {
        //         DEBUG_ASSERT(false, i, arg_tokens, tokens);
        //         LOG_WARN("Parsing error for asm function arguments");
        //     }
        //     if (split_arg_tokens_fn(tok) || ti + 1 == tokens.size()) {
        //         arg_tokens.at(i) = std::span{tokens.begin() + gsl::narrow_cast<std::ptrdiff_t>(last_start), len};
        //         auto str_start = tokens[last_start].str.data() - raw_str.data();
        //         arg_raw_strs.at(i) = raw_str.substr(str_start, tok.str.data() - raw_str.data() - str_start);
        //
        //         len = 0;
        //         last_start = ti + 1;
        //         ++i;
        //     } else {
        //         len++;
        //     }
        // }

        std::array stringized_args = std::apply(
            []<typename... Ts>(const Ts&... fn_args) { return std::array{stringize::str(fn_args)...}; }, args);

        return fmt::format("{}({})", function_name, fmt::join(stringized_args, ", "));
    }

    std::string str() const {
        if (Result<Ret>::has_value()) {
            if constexpr (std::same_as<Ret, void>) {
                return "void";
            } else {
                return stringize::str(Result<Ret>::value());
            }
        }
        return fmt::format("%#`<fg:red>{}`", Result<Ret>::error());
    }
};

template <typename T>
class AsmFunction
{
    static_assert(always_false_v<T>, "AsmFunction is not specialized for non-function types!");
};

namespace detail {

template <typename T>
concept IsUnwrappable = requires(const T& val) {
    { val.has_value() } -> std::same_as<bool>;
    { val.value() };
};

static_assert(IsUnwrappable<std::optional<int>>);
static_assert(IsUnwrappable<Expected<int>>);
static_assert(IsUnwrappable<Expected<>>);

/// Obtain the inner type of a T satisfying IsUnwrappable (e.g., std::optional or asmgrader::Expected)
template <typename T>
    requires IsUnwrappable<T>
using UnwrapInnerT = std::remove_reference_t<decltype(std::declval<T>().value())>;

static_assert(std::same_as<UnwrapInnerT<std::optional<int>>, int>);
static_assert(std::same_as<UnwrapInnerT<Expected<int>>, int>);
static_assert(std::same_as<UnwrapInnerT<Expected<>>, void>);

template <typename T>
struct UnwrapInnerOr;

template <typename T>
    requires(!IsUnwrappable<T>)
struct UnwrapInnerOr<T>
{
    using type = T;
};

template <typename T>
    requires(IsUnwrappable<T>)
struct UnwrapInnerOr<T>
{
    using type = UnwrapInnerT<T>;
};

template <typename T>
using UnwrapInnerOrT = UnwrapInnerOr<T>::type;

static_assert(std::same_as<UnwrapInnerOrT<std::optional<int>>, int>);
static_assert(std::same_as<UnwrapInnerOrT<const std::optional<int>>, const int>);
static_assert(std::same_as<UnwrapInnerOrT<int>, int>);
static_assert(std::same_as<UnwrapInnerOrT<int&>, int&>);

} // namespace detail

template <typename Ret, typename... Args>
class AsmFunction<Ret(Args...)>
{
public:
    AsmFunction(Program& prog, std::string name, std::uintptr_t address);
    AsmFunction(Program& prog, std::string name, ErrorKind resolution_err);

    template <typename... Ts>
        requires(sizeof...(Ts) == sizeof...(Args)) &&
                (true && ... && MemoryIOCompatible<detail::UnwrapInnerOrT<Ts>, Args>)
    AsmFunctionResult<Ret, Ts...> operator()(const Ts&... args) {
        static_assert((true && ... && std::copyable<std::decay_t<detail::UnwrapInnerOrT<Ts>>>),
                      "All arguments must be copyable");
        (check_ptr_arg<Ts>(), ...);

        // making copies of args...
        AsmFunctionResult<Ret, Ts...> res{{args...}, name_};

        if (resolution_err_.has_value()) {
            res.set_result(*resolution_err_);
            return res;
        }

        // try to unwrap any optional / expected / etc.
        std::optional unwrapped_args = try_unwrap_args(args...);

        if (!unwrapped_args) {
            res.set_result(ErrorKind::BadArgument);
            return res;
        }

        // this is a little messy
        // 1. resolve the overloaded fn
        // 2. bind the Program instance and address to the fn ptr
        // 3. call fn with unwrapped args tuple
        using FnType = Result<Ret> (Program::*)(decltype(address_), const detail::UnwrapInnerOrT<Ts>&...);
        const auto& prog_call_fn =
            std::bind_front(static_cast<FnType>(&Program::call_function<Ret(Args...)>), prog_, address_);
        auto call_res = std::apply(prog_call_fn, *unwrapped_args);

        res.set_result(std::move(call_res));

        return res;
    }

    const std::string& get_name() const { return name_; }

private:
    template <typename T>
    void check_ptr_arg() {
        using NormT = std::decay_t<T>;
        static_assert(!std::is_pointer_v<NormT> && !std::is_array_v<NormT>,
                      "Passing a raw pointer as argument for an asm function, which is probably not what you meant to "
                      "do. See docs on program memory for more info.");
    }

    /// If an argument satisfies `IsUnwrappable`, then checks whether it has an inner value.
    /// Else, returns true.
    template <typename T>
    bool check_valid_unwrappable(const T& arg) {
        if constexpr (detail::IsUnwrappable<T>) {
            return arg.has_value();
        }

        return true;
    }

    /// Automatically unwrap any type like std::optional or expected for nicer semantics when nesting function calls
    ///
    /// Say you have `AsmFuction`s `foo` and `bar`
    /// `foo(bar())` is allowed, assuming the return type of `bar` is compatible with `foo`'s parameter
    ///
    /// Any arguments for which `has_value()` evaluates to false will cause the function call to fail
    /// with a \ref ErrorKind::BadArgument error value.
    ///
    /// IMPORTANT: The lifetimes of `args` must extend to or past the lifetime of the return value of this function.
    template <typename... Ts>
    auto try_unwrap_args(const Ts&... args) -> std::optional<std::tuple<const detail::UnwrapInnerOrT<Ts>&...>> {
        if ((false || ... || !check_valid_unwrappable(args))) {
            return std::nullopt;
        }

        // unwrap a single argument, if it's unwrappable, or return the argument passed if not
        // will always return a reference, as lifetimes will be valid for this function's intended use-case.
        auto maybe_unwrap = []<typename T>(const T& arg) -> decltype(auto) {
            if constexpr (detail::IsUnwrappable<T>) {
                return arg.value();
            } else {
                return arg;
            }
        };

        return std::tuple<const detail::UnwrapInnerOrT<Ts>&...>{maybe_unwrap(args)...};
    }

    Program* prog_;

    std::uintptr_t address_;
    std::string name_;
    std::optional<ErrorKind> resolution_err_;
};

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, std::uintptr_t address)
    : prog_{&prog}
    , address_{address}
    , name_{std::move(name)} {}

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, ErrorKind resolution_err)
    : prog_{&prog}
    , address_{0x0}
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}

} // namespace asmgrader

template <typename Ret, typename... Ts>
struct fmt::formatter<::asmgrader::AsmFunctionResult<Ret, Ts...>> : formatter<::asmgrader::Result<Ret>>

{
};
