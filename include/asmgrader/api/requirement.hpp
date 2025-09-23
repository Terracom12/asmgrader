#pragma once

#include <asmgrader/api/asm_function.hpp>
#include <asmgrader/api/asm_symbol.hpp>
#include <asmgrader/api/decomposer.hpp>
#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/api/stringize.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/formatters/unknown.hpp>
#include <asmgrader/common/static_string.hpp>
#include <asmgrader/common/to_static_range.hpp>
#include <asmgrader/logging.hpp>
#include <asmgrader/meta/concepts.hpp>

#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/detail/mp_map_find.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/map.hpp>
#include <boost/type_index.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gsl/util>
#include <libassert/assert.hpp>
#include <range/v3/action/take_while.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/find_if_not.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop_while.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/split_when.hpp>
#include <range/v3/view/take_while.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <meta/meta.hpp>

namespace asmgrader {

/// Types and other utilities for representing expressions used in REQUIRE* statements
namespace exprs {

enum class ArityKind { Nullary = 0, Unary, Binary, Ternary };

template <typename OpFn, StaticString Rep, typename... Args>
struct NAryOp
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr NAryOp(std::tuple<Args...> args_, std::array<inspection::Tokenizer<>, sizeof...(Args)> arg_tokens_)
        : args{std::move(args_)}
        , arg_tokens{std::move(arg_tokens_)} {}

    // FIXME: Making a copy of everything
    std::tuple<Args...> args;
    std::array<inspection::Tokenizer<>, sizeof...(Args)> arg_tokens;

    using EvalResT = std::decay_t<std::invoke_result_t<OpFn, Args...>>;

    // FIXME: may be evaluated multiple times
    EvalResT eval() const { return std::apply(OpFn{}, args); }

    static constexpr std::string_view raw_str = Rep;

    static constexpr ArityKind arity{sizeof...(Args)};
};

/// For argument deduction purposes
template <template <typename...> typename Op, typename... Args>
constexpr auto make(std::array<inspection::Tokenizer<>, sizeof...(Args)> arg_tokens, Args&&... args) {
    return Op<Args...>{{std::forward<Args>(args)...}, arg_tokens};
}

template <typename Args>
using Noop = NAryOp<std::identity, "", Args>;

template <typename Arg>
using LogicalNot = NAryOp<std::logical_not<>, "!", Arg>;

template <typename Lhs, typename Rhs>
using Equal = NAryOp<std::equal_to<>, "==", Lhs, Rhs>;

template <typename Lhs, typename Rhs>
using NotEqual = NAryOp<std::not_equal_to<>, "!=", Lhs, Rhs>;

template <typename Lhs, typename Rhs>
using Less = NAryOp<std::less<>, "<", Lhs, Rhs>;

template <typename Lhs, typename Rhs>
using LessEqual = NAryOp<std::less_equal<>, "<=", Lhs, Rhs>;

template <typename Lhs, typename Rhs>
using Greater = NAryOp<std::greater<>, ">", Lhs, Rhs>;

template <typename Lhs, typename Rhs>
using GreaterEqual = NAryOp<std::greater_equal<>, ">=", Lhs, Rhs>;

template <typename T>
concept Operator = requires(T op) {
    { T::raw_str } -> std::convertible_to<std::string_view>;
    { T::arity } -> std::convertible_to<ArityKind>;
    { op.args } -> IsTemplate<std::tuple>;
    { op.arg_tokens } -> std::convertible_to<std::array<inspection::Tokenizer<>, static_cast<std::size_t>(T::arity)>>;
    { op.eval() };
} && std::tuple_size_v<decltype(T::args)> == static_cast<std::size_t>(T::arity);

// sanity checks
static_assert(Operator<LogicalNot<int>>);
static_assert(Operator<Equal<int, int>>);

namespace detail {

template <StaticString Str>
struct MapKeyT
{
};

template <typename T, typename U>
using StrToOpTMap = boost::mp11::mp_list<        //
    std::pair<MapKeyT<"==">, Equal<T, U>>,       //
    std::pair<MapKeyT<"!=">, NotEqual<T, U>>,    //
    std::pair<MapKeyT<"<">, Less<T, U>>,         //
    std::pair<MapKeyT<"<=">, LessEqual<T, U>>,   //
    std::pair<MapKeyT<">">, Greater<T, U>>,      //
    std::pair<MapKeyT<">=">, GreaterEqual<T, U>> //
    >;

static_assert(boost::mp11::mp_is_map<StrToOpTMap<int, int>>::value);

} // namespace detail

template <StaticString OpStr, typename T, typename U>
using OpStrToType = boost::mp11::mp_second<boost::mp11::mp_map_find<detail::StrToOpTMap<T, U>, detail::MapKeyT<OpStr>>>;

static_assert(std::same_as<OpStrToType<"!=", int, int>, NotEqual<int, int>>);

/// Representation of an expression with all components stringized
struct ExpressionRepr
{
    // FIXME: compiler no like [deletes default ctor of variant]
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    struct Repr
    {
        /// Representation of the expression as by `repr`
        stringize::StringizeResult repr;

        /// Representation of the expression's value as by `str`
        stringize::StringizeResult str;

        /// Original string (should be as generated by the # prepocessor operator)
        std::string_view raw_str;

        using TokenStream = std::vector<asmgrader::inspection::Token>;
        /// Tokenized raw_str see \ref Tokenizer
        TokenStream raw_str_tokens;

        /// This field will probably never be used, but it might be nice later for debug info
        boost::typeindex::type_index type_index;

        /// A heuristic of what type the value is. May not always be perfectly accurate
        enum class Type { String, Integer, Floating, Other };
        Type kind;

        /// Whether the expression is a literal value, or is composed of solely literal values.
        /// Examples:
        ///   123
        ///   (1 + 2) ^ 5
        // TODO: remove this?
        bool is_literal;
    };

    struct Value
    {
        Repr repr;
    };

    // forward decl for use in Expression
    struct Operator;

    using Expression = std::variant<Value, Operator>;

    /// The `str` field is the representation of the operator (e.g., '+', '!=').
    struct Operator
    {
        Repr repr;

        /// List of operands. Operator arity = `operands.size()` *Probably* will never support ops with higher arity
        /// than binary, but it's just a vector for future-proofing.
        std::vector<Expression> operands;
    };

    Expression expression;
};

} // namespace exprs

// TODO: Might want to optimize by making more use of views, as the primary use case will be in REQUIRE*

template <exprs::Operator Op>
class Requirement
{
public:
    static constexpr auto default_description = "<no description provided>";

    [[deprecated("====================================================================================================="
                 "=========================================================== !!!!!!!!!!!!!!!!! "
                 "Please consider providing a requirement description by using REQUIRE(..., \"<description here>\")"
                 " !!!!!!!!!!!!!!!!! "
                 "====================================================================================================="
                 "=========================================================== ")]]
    explicit Requirement(Op op)
        : Requirement(op, default_description) {}

    template <StaticString OpStr, typename... Ts>
    [[deprecated("====================================================================================================="
                 "=========================================================== !!!!!!!!!!!!!!!!! "
                 "Please consider providing a requirement description by using REQUIRE(..., \"<description here>\")"
                 " !!!!!!!!!!!!!!!!! "
                 "====================================================================================================="
                 "=========================================================== ")]]
    explicit Requirement(const DecomposedExpr<OpStr, Ts...>& decomposed_expr, const inspection::Tokenizer<>& tokens)
        : Requirement(decomposed_expr, tokens, default_description) {}

    template <StaticString OpStr, typename... Ts>
    explicit Requirement(const DecomposedExpr<OpStr, Ts...>& decomposed_expr, const inspection::Tokenizer<>& tokens,
                         std::string description)
        : Requirement(decomposed_to_op(decomposed_expr, tokens), std::move(description)) {}

    explicit Requirement(Op op, std::string description)
        : op_{op}
        , description_{std::move(description)}
        , res_{op.eval()} {}

    std::string get_description() const { return description_; }

    bool get_res() const { return static_cast<bool>(res_); }

    // TODO: Special case for enum / enum class enumerators. Count as literal heuristic: no operators other than `::`?
    exprs::ExpressionRepr get_expr_repr() const {
        // Only supporting a single unary or binary op for now, so this is pretty simple

        using enum exprs::ArityKind;

        if constexpr (Op::arity != Unary && Op::arity != Binary) {
            UNIMPLEMENTED("Operators that are not unary or binary are not supported");
        }

        // Special case for noop to just get the value
        if constexpr (IsTemplate<Op, exprs::Noop>) {
            using Arg0T = std::tuple_element<0, decltype(op_.args)>;
            std::string_view arg0_str = op_.arg_tokens.at(0).get_original();

            return exprs::ExpressionRepr{.expression = make_expr_value<Arg0T>(std::get<0>(op_.args), arg0_str)};
        } else {
            using ResultT = decltype(res_);
            exprs::ExpressionRepr::Repr repr{.repr = "==", //
                                             .str = stringize::str(res_),
                                             .raw_str = Op::raw_str,
                                             .raw_str_tokens =
                                                 inspection::Tokenizer<>{Op::raw_str} | ranges::to<std::vector>(),
                                             .type_index = boost::typeindex::type_id_with_cvr<ResultT>(),
                                             .kind = deduce_kind<ResultT>(),
                                             .is_literal = false};

            std::vector operands = std::apply(
                [this]<typename... TupleTypes>(const std::tuple<TupleTypes...>& /*for type deduction*/) {
                    return [this]<typename... Args>(Args&&... args) {
                        std::size_t idx = 0;
                        return std::vector{
                            make_expr_value<TupleTypes>(std::forward<Args>(args), op_.arg_tokens.at(idx++))...};
                    };
                }(op_.args),
                op_.args);

            return exprs::ExpressionRepr{.expression = exprs::ExpressionRepr::Operator{
                                             .repr = std::move(repr), .operands = std::move(operands)}};
        }
    }

private:
    template <StaticString OpStr, typename... Ts>
    static Op decomposed_to_op(const DecomposedExpr<OpStr, Ts...>& decomposed_expr,
                               const inspection::Tokenizer<>& tokens) {
        std::array<inspection::Tokenizer<>, sizeof...(Ts)> split_tokens_arr{};

        if constexpr (sizeof...(Ts) == 1) {
            split_tokens_arr.front() = tokens;
        } else {
            auto split_pos = gsl::narrow_cast<std::size_t>(
                ranges::find_if(tokens,
                                [](const inspection::Token& tok) { return tok.str == std::string_view{OpStr}; }) -
                ranges::begin(tokens));
            split_tokens_arr.at(0) = tokens.subseq(0, split_pos);
            split_tokens_arr.at(1) = tokens.subseq(split_pos + 1, tokens.size());
        }

        return Op(decomposed_expr.operands, split_tokens_arr);
    }

    template <typename Val>
    static consteval exprs::ExpressionRepr::Repr::Type deduce_kind() {
        using Repr = exprs::ExpressionRepr::Repr;
        using enum Repr::Type;

        using DecayT = std::decay_t<Val>;

        if constexpr (std::convertible_to<DecayT, std::string_view>) {
            return String;
        } else if constexpr (std::integral<DecayT>) {
            return Integer;
        } else if constexpr (std::floating_point<DecayT>) {
            return Floating;
        } else {
            return Other;
        }
    }

    template <typename OrigType, typename Val>
    exprs::ExpressionRepr::Repr make_repr_value(Val&& value, inspection::Tokenizer<> tokens,
                                                [[maybe_unused]] bool is_lvalue) const {

        // TODO: What about suffixes, which are parsed as identifiers
        // e.g., "Hello!"sv
        // that probably shouldn't be rendered seperately in output, but that's hard to show in general
        // maybe don't render it if it's a stdlib defined token (no _)?
        bool is_literal = !ranges::any_of(
            tokens, [](const inspection::Token& tok) { return tok.kind == inspection::Token::Kind::Identifier; });

        return {.repr = stringize::repr(tokens, tokens.get_original(), std::forward<Val>(value)),
                .str = stringize::str(std::forward<Val>(value)),
                .raw_str = tokens.get_original(),
                .raw_str_tokens = tokens | ranges::to<std::vector>(),
                .type_index = boost::typeindex::type_id_with_cvr<OrigType>(),
                .kind = deduce_kind<OrigType>(),
                .is_literal = is_literal};
    }

    // template <typename Val>
    // exprs::ExpressionRepr::Expression make_expr_value(Val& value, std::string_view raw_str) const {
    //     using Value = exprs::ExpressionRepr::Value;
    //
    //     return Value{.repr = make_repr_value(value, raw_str, true)};
    // }

    template <typename OrigType, typename Val>
    exprs::ExpressionRepr::Expression make_expr_value(Val&& value, inspection::Tokenizer<> tokens) const {
        using Value = exprs::ExpressionRepr::Value;

        bool is_lvalue{};

        if constexpr (std::is_lvalue_reference_v<OrigType>) {
            is_lvalue = true;
        } else {
            ASSERT(!std::is_reference_v<OrigType>, boost::typeindex::type_id_with_cvr<Val>());
            is_lvalue = false;
        }

        return Value{.repr = make_repr_value<OrigType>(std::forward<Val>(value), tokens, is_lvalue)};
    }

    Op op_;
    std::invoke_result_t<decltype(&Op::eval), Op> res_;
    std::string description_;

    static_assert(
        requires { static_cast<bool>(res_); }, "Requirement expressions must be convertible to bool (for now)");
};

/// Deduction guide for a single type decomposition expr
template <StaticString OpStr, typename T>
Requirement(DecomposedExpr<OpStr, T>&&, const inspection::Tokenizer<>&, std::string) -> Requirement<exprs::Noop<T>>;

/// Deduction guide for a binary type decomposition expr
template <StaticString OpStr, typename T, typename U>
Requirement(DecomposedExpr<OpStr, T, U>&&, const inspection::Tokenizer<>&, std::string)
    -> Requirement<exprs::OpStrToType<OpStr, T, U>>;

} // namespace asmgrader
