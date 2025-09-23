/// \file
/// Basic decomposition implementation, intended for use with the REQUIRE macro
/// Inspired by Catch2, or whomever first came up with the idea.

#include <asmgrader/common/class_traits.hpp>
#include <asmgrader/common/static_string.hpp>
#include <asmgrader/meta/always_false.hpp>

#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>

namespace asmgrader {

/// Just a tag type for operator overloading purposes
/// Intended use is on the lhs of operator<=
/// e.g.
///    Decomposer{} <= a < b + 1
/// Decomposes the expression  "a < b + 1" into the subexpressions "a" and "b + 1"
struct Decomposer
{
};

/// Stores references to rhs and lhs of a comparison expression, serving as an interface
/// to the "decomposed" expression.
template <StaticString Op, typename... Types>
struct DecomposedExpr : NonMovable
{
    constexpr explicit(false) DecomposedExpr(Types&&... args)
        : operands{std::forward<Types>(args)...} {}

    std::tuple<Types&&...> operands;

    static constexpr std::size_t arity = sizeof...(Types);
    static constexpr std::string_view op = Op;

    /// operator= is disabled via `static_assert(false, ...)`
    template <typename U>
    constexpr DecomposedExpr<Op, Types...>& operator=(const U& /*unused*/);
};

template <typename T>
constexpr DecomposedExpr<"", T> operator<=(Decomposer /*tag*/, T&& expr_lhs) {
    return {std::forward<T>(expr_lhs)};
}

////////// All comparison operator overloads for DecomposedExpr

template <typename T, typename U>
constexpr DecomposedExpr<"==", T, U> operator==(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

template <typename T, typename U>
constexpr DecomposedExpr<"!=", T, U> operator!=(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

template <typename T, typename U>
constexpr DecomposedExpr<"<", T, U> operator<(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

template <typename T, typename U>
constexpr DecomposedExpr<"<=", T, U> operator<=(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

template <typename T, typename U>
constexpr DecomposedExpr<">", T, U> operator>(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

template <typename T, typename U>
constexpr DecomposedExpr<">=", T, U> operator>=(DecomposedExpr<"", T>&& expr_lhs, U&& expr_rhs) {
    return {std::get<0>(std::move(expr_lhs).operands), std::forward<U>(expr_rhs)};
}

////////// Disabling lower-precedence operators and comparison operator chaining to keep it simple, and more readable
// Includes:
//   bitwise binary ops
//   logical binary ops
//   assignment and compound assignment ops
//   comma op

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage) - rational: static_assert requires a string literal pre-c++26
#define STATIC_ASSERT_PRE                                                                                              \
    "====================================================================================================="            \
    "=========================================================== !!!!!!!!!!!!!!!!! "                                   \
    "Operator "

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage) - rational: static_assert requires a string literal pre-c++26
#define STATIC_ASSERT_POST                                                                                             \
    " is not supported for decomposition! Wrap it in parentheses."                                                     \
    " !!!!!!!!!!!!!!!!! "                                                                                              \
    "====================================================================================================="            \
    "=========================================================== "

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage) - rational: static_assert requires a string literal pre-c++26
#define STATIC_ASSERT_CHAINED_POST                                                                                     \
    " is not supported to be chained with decomposition. Wrap it in parentheses, or fix the logic error."              \
    " !!!!!!!!!!!!!!!!! "                                                                                              \
    "====================================================================================================="            \
    "=========================================================== "

template <StaticString Op, typename... Ts>
template <typename U>
constexpr DecomposedExpr<Op, Ts...>& DecomposedExpr<Op, Ts...>::operator=(const U& /*unused*/) {
    static_assert(false, STATIC_ASSERT_PRE "=" STATIC_ASSERT_POST);
}

// NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved) - unnamed parameter

#define DISABLE_OPERATOR(op)                                                                                           \
    template <StaticString Op, typename... Ts, typename U>                                                             \
    constexpr void operator op(DecomposedExpr<Op, Ts...>&& /*expr_lhs*/, U&& /*expr_rhs*/) {                           \
        static_assert(always_false_v<U>, STATIC_ASSERT_PRE #op STATIC_ASSERT_POST);                                \
    }
#define DISABLE_CHAINED_OPERATOR(op)                                                                                   \
    template <StaticString Op, typename... Ts, typename U>                                                             \
    constexpr void operator op(DecomposedExpr<Op, Ts...>&& /*expr_lhs*/, U&& /*expr_rhs*/) {                           \
        static_assert(always_false_v<U>, STATIC_ASSERT_PRE #op STATIC_ASSERT_CHAINED_POST);                                        \
    }

DISABLE_OPERATOR(&);
DISABLE_OPERATOR(|);
DISABLE_OPERATOR(^);
DISABLE_OPERATOR(&&);
DISABLE_OPERATOR(||);
DISABLE_OPERATOR(+=);
DISABLE_OPERATOR(-=);
DISABLE_OPERATOR(*=);
DISABLE_OPERATOR(/=);
DISABLE_OPERATOR(%=);
DISABLE_OPERATOR(<<=);
DISABLE_OPERATOR(>>=);
DISABLE_OPERATOR(&=);
DISABLE_OPERATOR(^=);
DISABLE_OPERATOR(|=);
#define COMMA ,
DISABLE_OPERATOR(COMMA);
#undef COMMA

DISABLE_CHAINED_OPERATOR(==);
DISABLE_CHAINED_OPERATOR(!=);
DISABLE_CHAINED_OPERATOR(<);
DISABLE_CHAINED_OPERATOR(<=);
DISABLE_CHAINED_OPERATOR(>);
DISABLE_CHAINED_OPERATOR(>=);

#undef DISABLE_CHAINED_OPERATOR
#undef DISABLE_OPERATOR
#undef STATIC_ASSERT_POST
#undef STATIC_ASSERT_PRE

// NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved)

} // namespace asmgrader
