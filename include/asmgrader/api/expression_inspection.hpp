/// \file Implentation of inspection facilities for simple C++ expressions
/// Intended for usage with \ref `REQUIRE`
///
/// The ideas are heavily inspired by the libassert library, but the implementation is fully my own.
/// Why do this when an implementation already exists? Mostly cause parsing is kind of fun, and for a bit of learning.
#pragma once

#include <asmgrader/common/cconstexpr.hpp>
#include <asmgrader/common/formatters/macros.hpp>
#include <asmgrader/logging.hpp>

#include <fmt/format.h>
#include <gsl/narrow>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/find_if_not.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/any_view.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace asmgrader::inspection {

/// Token of a *very basic* C++ expression.
/// The primary use case is for rudemtary console syntax coloring.
///
/// Tokenization is essentially implemented as a fancy lexer, where there is no resultant syntax tree
/// and instead a simple 1D stream of tokens.
///
/// For instance, parsing the expression `x + y > "abc"` would generate the following stream of tokens:
///   Identifier, Operator, Identifier, Operator, StringLiteral
struct Token
{
    /// The kind of token.
    ///
    /// A modified version of EBNF is used to document enumerators.
    ///   - ".." is a contiguous alternation over ASCII encoded values, inclusive
    ///   - An 'i' after a string terminal means case insensitive
    ///   - Sequences are implicitly concatenated without ','
    ///   - '/' denotes removal of chars on rhs from the lhs
    ///     Ex: "abcdef" / "cd"   - this is equiv. to "abef"
    ///   - A "{<low>,<high>}" qualifier after a token means limited repitition, where `low` and `high` are both
    ///     inclusive and either may be ommitted.
    ///
    /// All definitions are implicitly defined with the maximal munch rule.
    /// https://en.wikipedia.org/wiki/Maximal_munch
    ///
    /// See this for the basic version: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
    enum class Kind {
        /// Under normal cases, this should be impossible.
        /// It's a saner option for a default, though, in case of a bad parse.
        Unknown,

        /// https://en.cppreference.com/w/cpp/language/string_literal.html
        ///
        /// StringLiteral =  [ strlike-prefix ] '"' { character } '''
        /// strlike-prefix = 'L' | 'u'i  [ '8' ]
        /// character = ANY_CHAR / '"\' | ESCAPE_SEQ
        StringLiteral,

        /// https://en.cppreference.com/w/cpp/language/string_literal.html
        ///
        /// RawStringLiteral = [ strlike-prefix ] 'R"' d-char-seq '(' { character } ')' d-char-seq '"'
        /// strlike-prefix = 'L' | 'u'i  [ '8' ]
        /// d-char-seq = ( character / '\()' - WHITESPACE ){,16}
        /// character = ANY_CHAR
        RawStringLiteral,

        /// https://en.cppreference.com/w/cpp/language/character_literal.html
        ///
        /// CharLiteral = [ strlike-prefix ] "'" char "'" | c-multi-char
        /// strlike-prefix = 'L' | 'u'i  [ '8' ]
        /// char = ANY_CHAR / "'\" | ESCAPE_SEQ
        /// c-multi-char = [ 'L' ] "'" { char } "'"
        ///
        /// I can't think of any good reasons to use a multi-char literal, but let's support it anyways as it's
        /// trivial to implement.
        CharLiteral,

        /// 'true' or 'false'. That's it.
        BoolLiteral,

        /// https://en.cppreference.com/w/cpp/language/integer_literal.html
        /// See \ref IntDecLiteral
        IntBinLiteral,

        /// https://en.cppreference.com/w/cpp/language/integer_literal.html
        /// See \ref IntDecLiteral
        /// This includes '0'
        IntOctLiteral,

        /// https://en.cppreference.com/w/cpp/language/integer_literal.html
        ///
        /// Not all terminals are defined, but they should be rather obvious anyways.
        ///
        /// IntLiteral = ( '0x'i hex-seq | dec-seq | '0' oct-seq | '0b'i bin-seq ) [ integer-suffix ]
        ///
        /// hex-digits = ( '0'..'9' | 'a'i..'f'i ) { '0'..'9' | 'a'i..'f'i | DIGIT_SEP }
        /// dec-digits = ( '1'..'9' ) { '0'..'9'  | DIGIT_SEP }
        /// oct-digits =  ( '0'..'7' ) { '0'..'8'  | DIGIT_SEP }
        /// bin-digits = ( '0' | '1' ) { '0' | '1' | DIGIT_SEP }
        ///
        /// integer-suffix = 'u'i  [ 'l'i | 'll'i ]
        /// DIGIT_SEP = "'"
        IntDecLiteral,

        /// https://en.cppreference.com/w/cpp/language/integer_literal.html
        /// See \ref IntDecLiteral
        IntHexLiteral,

        /// https://en.cppreference.com/w/cpp/language/floating_literal.html
        ///
        /// FloatLiteral = dec-value floating-point-suffix
        ///
        /// dec-value = dec-digits dec-exp
        ///           | dec-digits '.' [ dec-exp ]
        ///           | [ dec-digits ] '.' dec-digits [ dec-exp ]
        ///
        /// dec-digits = ( '1'..'9' ) { '0'..'9'  | DIGIT_SEP }
        /// dec-exp = 'e'i [ SIGN ] dec-seq
        ///
        /// SIGN = '+' | '-'
        ///
        /// floating-point-suffix = 'f'i | 'l'i
        FloatLiteral,

        /// https://en.cppreference.com/w/cpp/language/floating_literal.html
        ///
        /// FloatHexLiteral = hex-val floating-point-suffix
        ///
        /// hex-value = '0x'i hex-val-nopre
        /// hex-val-nopre = hex-digits hex-exp
        ///               | hex-digits '.' hex-exp
        ///               | [ hex-digits ] '.' hex-digits hex-exp
        ///
        /// hex-digits = ( '0'..'9' | 'a'i..'f'i ) { '0'..'9' | 'a'i..'f'i | DIGIT_SEP }
        /// hex-exp = 'p'i [ SIGN ] dec-seq
        ///
        /// SIGN = '+' | '-'
        ///
        /// floating-point-suffix = 'f'i | 'l'i
        FloatHexLiteral,

        /// https://en.cppreference.com/w/cpp/language/identifiers.html
        ///
        /// Identifier = ident-start { ident-start | '0'..'9' }
        /// ident-start = 'a'i..'z'i | '_'
        Identifier,

        /// Imperatively defined as:
        /// '{', '}'
        /// '(', ')' - when not as a function call
        /// '<', '>' - in template context
        Grouping,

        /// https://en.cppreference.com/w/cpp/language/operator_precedence.html
        /// (Note that, contrary to the title of this link, this impl has no concept of operator precedence)
        ///
        /// Only operators with 2 operands.
        /// Imperatively defined any of the symbols in the list below when they are not part of a previously defined
        /// token and do not meet the requirements for \ref Grouping. Perhaps a little confusingly, since the token
        /// stream is flat, operators like `a[]` produce 2 seperate operator tokens of '[' and ']'.
        ///
        ///   '::'
        ///   '.', '->'
        ///   '.*', '->*'
        ///   '+', '-', '*', '/', '%',
        ///   '<<', ">>', '^', '|', '&',
        ///   '&&', '||'
        ///   '==', '!=', '<=>', '<', '<=', '>', '>='
        ///   '=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=',
        ///   ','
        BinaryOperator,

        /// https://en.cppreference.com/w/cpp/language/operator_precedence.html
        /// (Note that, contrary to the title of this link, this impl has no concept of operator precedence)
        ///
        /// Unary, ternary, and (n>3)-ary (i.e., function call) operators
        /// Imperatively defined any of the symbols in the list below when they are not part of a previously defined
        /// token and do not meet the requirements for \ref Grouping. Perhaps a little confusingly, since the token
        /// stream is flat, operators like `a[]` produce 2 seperate operator tokens of '[' and ']'.
        ///   '++', '--'                                                    *** no distinction between pre and post
        ///   '(', ')', '[', ']'
        ///   '.*', '->*'
        ///   '+', '-',                                                     *** unary only
        ///   '~'
        ///   '!',
        ///   '*', '&'
        ///   'throw', 'sizeof', 'alignof', 'new', 'delete',
        ///   'const_cast', 'static_cast', 'dynamic_cast', 'reinterpret_cast',
        ///   '?', ':'
        ///   also includes literal operators
        // TODO: Maybe support alternate spellings like 'and', 'not', etc.
        Operator,

        /// Deliminates the end of the token sequence.
        /// Also serves to obtain a count of the number of token types, as this is guaranteed
        /// to be defined as the last enumerator.
        EndDelimiter
    };

    Kind kind;
    std::string_view str;

    constexpr bool operator==(const Token&) const = default;
};

constexpr std::string_view format_as(const Token::Kind token_kind) {
    using enum Token::Kind;
    switch (token_kind) {
    case Unknown:
        return "Unknown";
    case StringLiteral:
        return "StringLiteral";
    case RawStringLiteral:
        return "RawStringLiteral";
    case CharLiteral:
        return "CharLiteral";
    case IntBinLiteral:
        return "IntBinLiteral";
    case IntOctLiteral:
        return "IntOctLiteral";
    case IntDecLiteral:
        return "IntDecLiteral";
    case IntHexLiteral:
        return "IntHexLiteral";
    case FloatLiteral:
        return "FloatLiteral";
    case FloatHexLiteral:
        return "FloatHexLiteral";
    case Identifier:
        return "Identifier";
    case Grouping:
        return "Grouping";
    case Operator:
        return "Operator";
    case EndDelimiter:
        return "EndDelimiter";
    default:
        UNREACHABLE(token_kind);
    }
}

constexpr std::pair<Token::Kind, std::string_view> format_as(const Token& tok) {
    return {tok.kind, tok.str};
}

/// Bad or invalid parse exception type. May indicate an implementation bug,
/// or just an invalid expression.
class ParsingError : public std::exception
{
public:
    // Using constexpr where possible for the unlikely possibility of eventually updating to C++26
    // See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3068r6.html

    explicit ParsingError(std::string msg)
        : msg_{std::move(msg)} {}

    ParsingError(std::string msg, std::string_view stream_state)
        : msg_{std::move(msg)}
        , stream_state_{stream_state} {}

    ParsingError(std::string msg, std::string_view stream_state, Token::Kind token_kind)
        : msg_{std::move(msg)}
        , stream_state_{stream_state}
        , token_kind_{token_kind} {}

    const char* what() const noexcept override { return get_pretty().c_str(); }

    constexpr const std::string& msg() const noexcept { return msg_; }

    constexpr const std::optional<std::string>& stream_state() const noexcept { return stream_state_; }

    constexpr const std::optional<Token::Kind>& token_kind() const noexcept { return token_kind_; }

    /// Return a human-readable "pretty" stringified version of the exception
    std::string pretty() const { return get_pretty(); }

private:
    std::string& get_pretty() const {
        static std::string pretty_cache;

        if (pretty_cache.empty()) {
            std::string_view stream_state_str = stream_state_ ? std::string_view{*stream_state_} : "<unknown>";
            std::string_view token_kind_str = token_kind_ ? format_as(*token_kind_) : "<unknown>";

            pretty_cache = fmt::format("{} : state={:?}, token={}", msg_, stream_state_str, token_kind_str);
        }

        return pretty_cache;
    }

    /// A message as to why parsing failed
    std::string msg_;
    /// Optional state of the stream when failure occured
    std::optional<std::string> stream_state_;
    /// Optional token kind we were attempting to parse when failure occured
    std::optional<Token::Kind> token_kind_;
};

inline const std::exception& format_as(const ParsingError& err) {
    return err;
}

/// Recursive descent parser implementation details
namespace tokenize {

// I'm pretty rusty so heavy inspiration was taken from https://craftinginterpreters.com/parsing-expressions.html
//
// This implementation is not extremely efficient, but my use case is all in constexpr-contexts, so it doesn't
// matter much at all.
//
// The assumption is made that the token stream is in a valid state upon the call of every function
// For instance, if we see that the stream starts with a ', we assume it's a char literal and not a
// seperator within a digit literal.
// It is also assumed that the token stream is syntactically valid.

///////////////// string_view utility functions

/// A substr of `str` up to the first occurrence of `token`,
/// or the entirety of `str` if `token` is not found
[[nodiscard]] constexpr std::string_view substr_to(std::string_view str, auto token) {
    auto pos = str.find(token);
    return str.substr(0, pos);
}

static_assert(substr_to("abcd", 'c') == "ab");
static_assert(substr_to("abc ef ab", "ef") == "abc ");

/// \overload
/// A substr of `str` up to the first character that satisfies `pred`,
/// or the entirety of `str` if `pred` is never satisfied
/// In essence, performs 'take while not' (a.k.a. 'take until')
[[nodiscard]] constexpr std::string_view substr_to(std::string_view str, std::invocable<char> auto pred) {
    auto iter = ranges::find_if(str, pred);

    if (iter == str.end()) {
        return str;
    }

    return str.substr(0, static_cast<std::size_t>(iter - str.begin()));
}

static_assert(substr_to("abc0123 ab", isdigit) == "abc");
static_assert(substr_to("0123 ab", std::not_fn(isdigit)) == "0123");

/// A substr of `str` past all characters satisfying `pred`
/// In essence, performs 'drop while'
[[nodiscard]] constexpr std::string_view substr_past(std::string_view str, auto what) {
    std::size_t skip_len{};
    if constexpr (std::invocable<decltype(what), char>) {
        skip_len = substr_to(str, std::not_fn(what)).size();
    } else {
        skip_len = str.find(what) + 1;
    }

    return str.substr(skip_len);
}

static_assert(substr_past("abc0123 ab", isalpha) == "0123 ab");
static_assert(substr_past("abc0123 ab", isalnum) == " ab");
static_assert(substr_past("abc0123 ab", isdigit) == "abc0123 ab");
static_assert(substr_past("", isdigit) == "");

///////////////// character stream structure

using enum Token::Kind;

constexpr struct CaseInsensitiveTag
{
} case_insensitive{};

class Stream
{
public:
    /// Primarily used to support Token::Kind::Grouping
    struct StreamContext
    {
        std::span<Token> prevs;

        constexpr Token::Kind last_kind() const {
            if (prevs.empty()) {
                return Unknown;
            }
            return prevs.back().kind;
        }
    };

    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    StreamContext ctx{};

    constexpr explicit(false) Stream(const char* string)
        : str_{string} {}

    constexpr explicit(false) Stream(std::string_view string)
        : str_{string} {}

    /// This overload is just used for writing tests
    constexpr Stream(std::string_view string, StreamContext context)
        : ctx{context}
        , str_{string} {}

    constexpr std::size_t size() const { return str().size(); }

    constexpr bool empty() const { return str().empty(); }

    /// Peek at the first character of the stream
    constexpr char peek() const {
        if (str().empty()) {
            throw ParsingError("called peek with empty stream");
        }
        return str().at(0);
    }

    /// \overload
    /// Peek at the first n characters of the stream
    constexpr std::string_view peek(std::size_t n) const {
        if (n > size()) {
            throw ParsingError(fmt::format("called peek(n) where n > size() ({} > {})", n, size()));
        }
        return str().substr(0, n);
    }

    // FIXME: These are confusing. Change to more idiomatic forms 'take_while', 'drop_while', etc.

    /// \ref substr_to
    constexpr std::string_view peek_until(auto what) const { return substr_to(str(), what); }

    constexpr std::string_view peek_while(std::invocable<char> auto pred) const {
        return peek_until(std::not_fn(pred));
    }

    constexpr std::string_view peek_through(auto what) const {
        auto past_sz = peek_past(what).size();
        return str().substr(0, str().size() - past_sz);
    }

    /// \ref substr_past
    constexpr std::string_view peek_past(auto what) const { return substr_past(str(), what); }

    /// Same as \ref peek, except it also mutates the stream
    constexpr std::string_view consume(std::size_t n) {
        auto res = peek(n);
        idx_ += res.size();
        return res;
    }

    /// Attempt to consume str at the beginning of the stream, iff it actually exists
    /// at the beginning
    /// \param str Either a string or char to attempt to consume
    /// \returns Whether str was successfully consumed
    template <typename StrLike>
        requires std::convertible_to<StrLike, std::string_view> || std::same_as<StrLike, char>
    constexpr bool consume(StrLike str) {
        if (!starts_with(str)) {
            return false;
        }

        if constexpr (std::same_as<char, decltype(str)>) {
            idx_ += 1;
        } else {
            idx_ += std::string_view{str}.size();
        }

        return true;
    }

    constexpr bool consume(auto what, CaseInsensitiveTag /*unused*/) {
        if (!starts_with(what, case_insensitive)) {
            return false;
        }

        if constexpr (std::same_as<char, decltype(what)>) {
            idx_ += 1;
        } else {
            idx_ += std::string_view{what}.size();
        }

        return true;
    }

    /// Same as \ref peek_until, except it also mutates the stream
    constexpr std::string_view consume_until(auto what) {
        auto res = peek_until(what);
        idx_ += res.size();
        return res;
    }

    /// Same as \ref peek_through, except it also mutates the stream
    constexpr std::string_view consume_through(auto what) {
        auto res = peek_through(what);
        idx_ += res.size();
        return res;
    }

    /// Same as \ref peek_while, except it also mutates the stream
    constexpr std::string_view consume_while(std::invocable<char> auto pred) {
        auto res = peek_while(pred);
        idx_ += res.size();
        return res;
    }

    /// Whether the stream starts with `what` (accepts same as `std::string_view::starts_with`)
    constexpr bool starts_with(auto what) const { return str().starts_with(what); }

    constexpr bool starts_with(char chr, CaseInsensitiveTag /*unused*/) const {
        return !str().empty() && tolower(str().front()) == tolower(chr);
    }

    constexpr bool starts_with(std::string_view string, CaseInsensitiveTag /*unused*/) const {
        return size() >= string.size() &&
               ranges::equal(string, str().substr(0, string.size()), std::equal_to{}, tolower, tolower);
    }

    constexpr std::string_view str() const { return str_.substr(idx_); }

private:
    std::string_view str_;
    std::size_t idx_ = 0;
};

template <std::size_t N>
constexpr auto make_rev_size_sorted(const std::string_view (&arr)[N]) {
    std::array<std::string_view, N> array = std::to_array(arr);

    ranges::sort(array, std::greater{}, &std::string_view::size);

    return array;
}

constexpr auto binary_operator_tokens = make_rev_size_sorted({
    "::", ",",                                                           //
    ".",  "->",                                                          //
    ".*", "->*",                                                         //
    "+",  "-",   "*",   "/",  "%",                                       //
    "<<", ">>",  "^",   "|",  "&",                                       //
    "&&", "||",                                                          //
    "==", "!=",  "<=>", "<",  "<=", ">",  ">=",                          //
    "=",  "+=",  "-=",  "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=" //
});

constexpr auto operator_tokens = make_rev_size_sorted({
    "++", "--",                                                      //
    "(", ")", "[", "]",                                              //
    "+", "-",                                                        //
    "~", "!",                                                        //
    "*", "&",                                                        //
    "throw", "sizeof", "alignof", "new", "delete",                   //
                                                                     //
    "const_cast", "static_cast", "dynamic_cast", "reinterpret_cast", //
    "::", "?", ":"                                                   //
});

// Only 5 operands, '+' '-' '*' '&' and '::', should be present in both operator_tokens and binary_operator_tokens
static_assert(ranges::count_if(operator_tokens, std::bind_front(ranges::contains, binary_operator_tokens)) == 5);

constexpr auto grouping_tokens = std::to_array<char>({
    '{', '}', //
    '(', ')', //
    '<', '>'  //
});

///////////////// token matching implementation

/// Whether the entirety of `str` is a strlike-prefix
/// See \ref Token::Kind::StringLiteral for details
constexpr bool is_strlike_prefix(std::string_view str) {
    if (str.empty() || str.size() > 2) {
        return false;
    }

    if (str.size() == 1 && str.starts_with('L')) {
        return true;
    }

    if (tolower(str.at(0)) == 'u') {
        return str.size() == 1 || str.at(1) == '8';
    }

    return false;
}

static_assert(is_strlike_prefix("L") && is_strlike_prefix("u") && is_strlike_prefix("U8"));
static_assert(!is_strlike_prefix("au8") && !is_strlike_prefix("\"L"));

/// Whether the entirety of `str` is an integer-suffix
/// See \ref Token::Kind::IntLiteral for details
constexpr bool is_int_suffix(std::string_view str) {
    if (str.empty() || str.size() > 3) {
        return false;
    }

    // check for unsigned spec
    if (tolower(str.front()) == 'u') {
        str.remove_prefix(1);

        if (str.empty()) {
            return true;
        }
    }

    // check for long / long long spec
    if (tolower(str.front()) == 'l') {
        str.remove_prefix(1);

        if (str.empty()) {
            return true;
        }
    }

    return str.size() == 1 && tolower(str.front()) == 'l';
}

static_assert(is_int_suffix("L"));
static_assert(is_int_suffix("u"));
static_assert(is_int_suffix("ul"));
static_assert(is_int_suffix("LL"));
static_assert(is_int_suffix("ULL"));
static_assert(!is_int_suffix("lu") && !is_int_suffix("Lu"));

/// Returns a functor to check for an identifier for a stream of characters
/// Does not verify whether the ident is a reserved token or not.
/// See \ref Token::Kind::Identifier for details
constexpr auto is_ident_like() {
    return [first = true](char c) mutable {
        if (std::exchange(first, false) && !isalpha(c) && c != '_') {
            return false;
        }

        return isalnum(c) || c == '_';
    };
}

/// Exactly as named. Sep = '
constexpr auto digit_or_sep(char c) {
    return isdigit(c) || c == '\'';
}

/// Exactly as named. Sep = '
constexpr auto xdigit_or_sep(char c) {
    return isxdigit(c) || c == '\'';
}

// Yes, I know \overload is not technically accurate for the following definitions,
// but it groups the functions nicely in documentation.

/// Check whether the start of `stream` matches a token kind
/// Assumes that there is no leading whitespace in `stream`
///
/// \param stream The character stream
/// \tparam Kind The kind of token to match to
template <Token::Kind Kind>
constexpr bool matches([[maybe_unused]] const Stream& stream) {
    return false;
}

/// \overload
/// Whether the start of `stream` is a string literal token
/// See \ref Token::Kind::StringLiteral for details
template <>
constexpr bool matches<StringLiteral>(const Stream& stream) {
    std::string_view potential_strlike_prefix = stream.peek_until('"');

    // if the prefix is empty, then the stream just starts with " (it's a string)
    if (potential_strlike_prefix.empty()) {
        return true;
    }

    return is_strlike_prefix(potential_strlike_prefix);
}

static_assert(matches<StringLiteral>(R"("")"));
static_assert(matches<StringLiteral>(R"(u8"")"));
static_assert(matches<StringLiteral>(R"(L"")"));
static_assert(!matches<StringLiteral>(R"(a"")"));

/// \overload
/// Whether the start of `stream` is a raw string literal token
/// See \ref Token::Kind::RawStringLiteral for details
template <>
constexpr bool matches<RawStringLiteral>(const Stream& stream) {
    std::string_view potential_strlike_prefix = stream.peek_until("R\"");

    // if the prefix is empty, then the stream just starts with R" (it's a raw string literal)
    if (potential_strlike_prefix.empty()) {
        return true;
    }

    return is_strlike_prefix(potential_strlike_prefix);
}

static_assert(matches<RawStringLiteral>(R"(R"")"));
static_assert(matches<RawStringLiteral>(R"(u8R"")"));
static_assert(matches<RawStringLiteral>(R"(LR"")"));
static_assert(!matches<RawStringLiteral>(R"("")"));

/// \overload
/// Whether the start of `stream` is a char literal token
/// See \ref Token::Kind::CharLiteral for details
template <>
constexpr bool matches<CharLiteral>(const Stream& stream) {
    std::string_view potential_strlike_prefix = stream.peek_until('\'');

    // if the prefix is empty, then the stream just starts with ' (it's a char literal)
    if (potential_strlike_prefix.empty()) {
        return true;
    }

    return is_strlike_prefix(potential_strlike_prefix);
}

static_assert(matches<CharLiteral>("''"));
static_assert(matches<CharLiteral>("u''"));
static_assert(matches<CharLiteral>("u8''"));
static_assert(!matches<CharLiteral>("+''"));

/// \overload
/// See \ref Token::Kind::BoolLiteral for details
template <>
constexpr bool matches<BoolLiteral>(const Stream& stream) {
    auto leading = stream.peek_through(is_ident_like());

    return leading == "true" || leading == "false";
}

static_assert(matches<BoolLiteral>("true"));
static_assert(matches<BoolLiteral>("false"));
static_assert(!matches<BoolLiteral>("_false"));
static_assert(!matches<BoolLiteral>("false_"));

/// \overload
/// Whether the start of `stream` is a binary integer literal token
/// See \ref Token::Kind::IntBinLiteral for details
template <>
constexpr bool matches<IntBinLiteral>(const Stream& stream) {
    if (!stream.starts_with("0b", case_insensitive)) {
        return false;
    }

    return stream.size() > 2 && isdigit(stream.str().at(2));
}

/// \overload
/// Whether the start of `stream` is a hexadecimal integer literal token
/// See \ref Token::Kind::IntHexLiteral for details
template <>
constexpr bool matches<IntHexLiteral>(const Stream& stream) {
    if (stream.size() <= 2 || !stream.starts_with("0x", case_insensitive)) {
        return false;
    }

    std::string_view after_prefix = stream.str().substr(2);

    if (!isxdigit(after_prefix.front())) {
        return false;
    }

    Stream after_digits = substr_past(after_prefix, xdigit_or_sep);

    // Hex int digit-seq proceeded by a 'p' is ALWAYS interpreted as an exponent,
    // as it is defined as such as part of the lexical grammar
    return !after_digits.starts_with('p', case_insensitive) && !after_digits.starts_with('.');
}

/// \overload
/// Whether the start of `stream` is an octal integer literal token
/// See \ref Token::Kind::IntOctLiteral for details
template <>
constexpr bool matches<IntOctLiteral>(const Stream& stream) {
    if (!stream.starts_with('0')) {
        return false;
    }

    Stream after_digits = stream.peek_past(digit_or_sep);

    // Non-hex int digit-seq proceeded by a 'e' is ALWAYS interpreted as an exponent,
    // as it is defined as such as part of the lexical grammar
    // int digit-seq proceeded by a 'x' is ALWAYS interpreted as a hex literal, for same reason as above
    return !after_digits.starts_with('x', case_insensitive) && !after_digits.starts_with('e', case_insensitive) &&
           !after_digits.starts_with('.');
}

/// \overload
/// Whether the start of `stream` is a decimal integer literal token
/// Written in terms of other int literals.
/// See \ref Token::Kind::IntDecLiteral for details
template <>
constexpr bool matches<IntDecLiteral>(const Stream& stream) {
    if (stream.empty() || !isdigit(stream.peek())) {
        return false;
    }

    if (matches<IntBinLiteral>(stream) || matches<IntOctLiteral>(stream) || matches<IntHexLiteral>(stream)) {
        return false;
    }

    Stream after_digits = stream.peek_past(digit_or_sep);

    // Non-hex int digit-seq proceeded by a 'e' is ALWAYS interpreted as an exponent,
    // as it is defined as such as part of the lexical grammar
    // int digit-seq proceeded by a 'x' is ALWAYS interpreted as a hex literal, for same reason as above
    return !after_digits.starts_with('x', case_insensitive) && !after_digits.starts_with('e', case_insensitive) &&
           !after_digits.starts_with('.');
}

static_assert(matches<IntDecLiteral>("123"));
static_assert(matches<IntDecLiteral>("123+"));
static_assert(matches<IntDecLiteral>("123u"));
static_assert(matches<IntHexLiteral>("0xAEF102"));
static_assert(matches<IntHexLiteral>("0xAEF102ull"));
static_assert(matches<IntBinLiteral>("0b1010"));
static_assert(matches<IntOctLiteral>("0"));
static_assert(matches<IntOctLiteral>("01'2"));
static_assert(matches<IntOctLiteral>("0'1'2345'123"));
static_assert(matches<IntHexLiteral>("0x1'A'b3F5'123"));
static_assert(!matches<IntOctLiteral>("'0"));
static_assert(!matches<IntDecLiteral>("0x123p3"));
static_assert(!matches<IntDecLiteral>("0x123.0x23"));
static_assert(!matches<IntDecLiteral>("0x123.0x23p3"));
static_assert(!matches<IntHexLiteral>("0x123p3"));
static_assert(!matches<IntHexLiteral>("0x123.0x23"));
static_assert(!matches<IntHexLiteral>("0x123.0x23p3"));
static_assert(!matches<IntOctLiteral>("0x123"));
static_assert(!matches<IntOctLiteral>("0XABC"));
static_assert(!matches<IntOctLiteral>("0.123"));
static_assert(!matches<IntDecLiteral>("10.123"));
static_assert(!matches<IntDecLiteral>(".123"));
static_assert(!matches<IntOctLiteral>("0.f"));
static_assert(!matches<IntDecLiteral>("1e5"));
static_assert(!matches<IntHexLiteral>("0x15p3"));

/// \overload
/// Whether the start of `stream` is a hexadecimal floating point literal token
/// See \ref Token::Kind::FloatLiteral for details
template <>
constexpr bool matches<FloatHexLiteral>(const Stream& stream) {
    // Only 2 valid forms for the start of a hex floating-point literal
    if (stream.starts_with("0x", case_insensitive)) {
        return !matches<IntHexLiteral>(stream);
    }

    return stream.starts_with(".0x", case_insensitive);
}

/// \overload
/// Whether the start of `stream` is a floating point literal token
/// See \ref Token::Kind::FloatLiteral for details
template <>
constexpr bool matches<FloatLiteral>(const Stream& stream) {
    if (matches<FloatHexLiteral>(stream)) {
        return false;
    }
    // stream starts with '.' and then a digit
    if (stream.starts_with('.') && stream.size() > 1 && isdigit(stream.str().at(1))) {
        return true;
    }

    if (!isdigit(stream.peek())) {
        return false;
    }

    return !matches<IntBinLiteral>(stream) && !matches<IntOctLiteral>(stream) && !matches<IntDecLiteral>(stream) &&
           !matches<IntHexLiteral>(stream);
}

static_assert(matches<FloatLiteral>(".123"));
static_assert(matches<FloatLiteral>(".123e52"));
static_assert(matches<FloatLiteral>(".123e+52"));
static_assert(matches<FloatLiteral>(".123e-52"));
static_assert(matches<FloatLiteral>("0.123"));
static_assert(matches<FloatLiteral>("10.123e31"));
static_assert(matches<FloatLiteral>("12e42"));
static_assert(matches<FloatLiteral>("10.123f"));
static_assert(matches<FloatLiteral>("0.fl"));
static_assert(matches<FloatLiteral>("0'123.1'2345'6fl"));
static_assert(matches<FloatLiteral>(".0FL"));

static_assert(matches<FloatHexLiteral>("0xAEFp3"));
static_assert(matches<FloatHexLiteral>("0xAEFp+3"));
static_assert(matches<FloatHexLiteral>("0xAEFp-3"));
static_assert(matches<FloatHexLiteral>("0xAEF.0x123p3"));
static_assert(matches<FloatHexLiteral>(".0x123p3"));
static_assert(matches<FloatHexLiteral>("0x12'1EF.p0x123"));
static_assert(matches<FloatHexLiteral>("0x123.0xABCp10"));

static_assert(!matches<FloatLiteral>("0xAEFp3"));
static_assert(!matches<FloatLiteral>("0xAEFp+3"));
static_assert(!matches<FloatLiteral>("0xAEFp-3"));
static_assert(!matches<FloatLiteral>("0xAEF.0x123p3"));
static_assert(!matches<FloatLiteral>(".0x123p3"));
static_assert(!matches<FloatLiteral>("0x12'1EF.p0x123"));
static_assert(!matches<FloatLiteral>("0x123.0xABCp10"));
static_assert(!matches<FloatLiteral>("123"));
static_assert(!matches<FloatLiteral>("0"));
static_assert(!matches<FloatLiteral>("0b10"));
static_assert(!matches<FloatHexLiteral>("0b10"));
static_assert(!matches<FloatHexLiteral>("0xAB10"));

/// \overload
/// Whether the start of `stream` is an identifier token
/// See \ref Token::Kind::Identifier for details
template <>
constexpr bool matches<Identifier>(const Stream& stream) {
    // Make sure that the token is not an operator (new, etc.)
    auto full_token = stream.peek_through(is_ident_like());

    if (full_token.empty()) {
        return false;
    }

    return !ranges::contains(operator_tokens, full_token) && !matches<BoolLiteral>(stream);
}

static_assert(matches<Identifier>("abc"));
static_assert(matches<Identifier>("_"));
static_assert(matches<Identifier>("_abc"));
static_assert(matches<Identifier>("_12abc"));
static_assert(!matches<Identifier>("1abc"));
static_assert(!matches<Identifier>("1_abc"));
static_assert(!matches<Identifier>("+_abc"));
static_assert(!matches<Identifier>(".123"));
static_assert(!(matches<Identifier>("new") || matches<Identifier>("sizeof")));

/// \overload
/// Whether the start of `stream` is an identifier token
/// See \ref Token::Kind::Identifier for details
template <>
constexpr bool matches<Grouping>(const Stream& stream) {
    char tok = stream.peek();

    if (!ranges::contains(grouping_tokens, tok)) {
        return false;
    }

    // these are always grouping symbols
    if (tok == '{' || tok == '}') {
        return true;
    }

    if (tok == '(') {
        return stream.ctx.last_kind() != Identifier;
    }

    // a ')' is a closing grouping symbol iff an unmatched opening '(' grouping symbol
    // is found MORE RECENTLY THAN an unmatched opening '(' operator
    if (tok == ')') {
        auto last_grouping_opening = ranges::find(stream.ctx.prevs, Token{.kind = Grouping, .str = "("});
        // no opening token -> nothing to match to
        if (last_grouping_opening == ranges::end(stream.ctx.prevs)) {
            return false;
        }

        auto num_operator_opening = ranges::count(stream.ctx.prevs, Token{.kind = Operator, .str = "("});
        auto num_operator_closing = ranges::count(stream.ctx.prevs, Token{.kind = Operator, .str = ")"});

        // there cannot be more closing than opening. That would imply a parsing error
        if (num_operator_closing > num_operator_opening) {
            throw ParsingError("closing ')' ops > opening '(' ops", stream.str(), Grouping);
        }

        // no unmatched operator opening. We must be matching to the previous grouping symbol
        if (num_operator_opening == num_operator_closing) {
            return true;
        }

        auto last_operator_opening = ranges::find(stream.ctx.prevs, Token{.kind = Operator, .str = "("});

        return last_grouping_opening > last_operator_opening;
    }

    // very basic heuristic for checking if '<' / '>' are being used to surround tparams:
    //   for '<': if an excess '>' is found further in the stream, before any other '<' chars, there is no logical
    //   operator in-between, there is no opening parenthesis in-between, and the chars up to the first alnum/ws/'_'
    //   do not match an operator
    //   for '>': an unmatched '<' exists

    // FIXME: This is not worth implementing properly right now
    //        as long as expressions like the following forms can be parsed:
    //        `ident<simple-expr>`
    //        `ident<simple-expr> </> ident<simple-expr>`
    //        `ident<simple-expr> </> ident`
    //        `ident </> ident<simple-expr>`
    // TODO: handle bitshift ambiguity and '<' / '>' operators nested within a tparam
    if (tok == '<') {
        auto full_token = stream.peek_until([](char c) { return isalnum(c) || isblank(c) || c == '_'; });
        if (full_token.size() > 1 &&
            (ranges::contains(binary_operator_tokens, full_token) || ranges::contains(operator_tokens, full_token))) {
            return false;
        }

        auto first_logical_op = std::min(stream.str().find("&&"), stream.str().find("||"));
        auto first_opening_paren = stream.str().find('(');
        auto next_closing_angled = stream.str().find('>');

        if (next_closing_angled > first_logical_op || next_closing_angled > first_opening_paren) {
            return false;
        }

        auto next_opening_angled = stream.str().substr(1).find('<');

        return next_opening_angled > next_closing_angled;
    }

    if (tok == '>') {
        auto num_angled_opening = ranges::count(stream.ctx.prevs, Token{.kind = Grouping, .str = "<"});
        auto num_angled_closing = ranges::count(stream.ctx.prevs, Token{.kind = Grouping, .str = ">"});

        // there cannot be more closing than opening. That would imply a parsing error
        // We're not going to assert this for now though, as the heuristic is very basic and will
        // have a lot of errors.

        if (!std::is_constant_evaluated() && num_angled_closing > num_angled_opening) {
            LOG_WARN("Parsing error for '<' '>' grouping tokens (# opening < # closing). Context: (o={},c={}) "
                     "stream={:?}, prevs={}",
                     num_angled_opening, num_angled_closing, stream.str(), stream.ctx.prevs);
        }

        return num_angled_opening > num_angled_closing;
    }

    UNREACHABLE(tok, stream, stream.ctx.prevs);
}

// tests with context can be found at parse<Grouping>

/// \overload
/// Whether the start of `stream` is a binary operator token
/// See \ref Token::Kind::BinaryOperator for details
template <>
constexpr bool matches<BinaryOperator>(const Stream& stream) {
    // Grouping tokens have the same chars as operators, except require more context checks,
    // so give precedence to identifying them
    if (matches<Grouping>(stream)) {
        return false;
    }

    // To abide by the maximal munch rule, prioritizing bigger tokens over samller ones
    // e.g., "-=" is always picked over "-"
    auto max_munch_tokens = binary_operator_tokens;
    ranges::sort(max_munch_tokens, std::greater{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);
    auto iter = ranges::find_if(max_munch_tokens, check_stream_starts);

    if (iter == ranges::end(max_munch_tokens)) {
        return false;
    }

    // Logic to seperate binary-operators from any other arity is defined in matches<Operator>

    // the only 3 operators that could be unary or binary
    if (!ranges::contains(operator_tokens, *iter)) {
        return true;
    }

    // At the start of the stream -> MUST be a unary operator
    if (stream.ctx.prevs.empty()) {
        return false;
    }

    // the last token was an operator -> this one MUST be a unary operator
    if (Token::Kind kind = stream.ctx.prevs.back().kind; kind == Operator || kind == BinaryOperator) {
        return false;
    }

    return true;
}

/// \overload
/// Whether the start of `stream` is an operator token
/// See \ref Token::Kind::Operator for details
template <>
constexpr bool matches<Operator>(const Stream& stream) {
    // Grouping tokens have the same chars as operators, except require more context checks,
    // so give precedence to identifying them
    if (matches<Grouping>(stream)) {
        return false;
    }

    // To abide by the maximal munch rule, prioritizing bigger tokens over samller ones
    // e.g., "-=" is always picked over "-"
    auto max_munch_tokens = operator_tokens;
    ranges::sort(max_munch_tokens, std::greater{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);

    auto iter = ranges::find_if(max_munch_tokens, check_stream_starts);

    if (iter == ranges::end(max_munch_tokens)) {
        return false;
    }

    // the only 2 operators that could be unary or binary
    if (ranges::contains(binary_operator_tokens, *iter)) {
        return !matches<BinaryOperator>(stream);
    }

    return true;
}

static_assert(matches<Operator>("+ 123"));
static_assert(matches<Operator>(":: 123"));
static_assert(matches<Operator>("? 123"));
static_assert(matches<Operator>("sizeof 123"));
static_assert(!matches<Operator>("(123)"));
static_assert(!matches<Operator>("{ 123"));
static_assert(!matches<Operator>("|| 123"));

// tests for binary operators are impossible to do without context.
// They may be found in test_expression_inspection.cpp

///////////////// token parsing implementation

template <Token::Kind Kind>
constexpr std::string_view test_parse(std::string_view str) {
    Stream stream{str};

    return parse<Kind>(stream);
}

/// Parse the token of Kind from the start of the stream.
/// Assumes that the stream actually starts with a token of Kind.
/// (Check with \ref match)
///
/// \param stream The character stream. This is mutated, consuming parsed tokens.
/// \tparam Kind The kind of token to parse
/// \return A view of the token content
template <Token::Kind Kind>
constexpr std::string_view parse([[maybe_unused]] Stream& stream) {
    return "";
}

/// \overload
/// See \ref Token::Kind::StringLiteral for details
template <>
constexpr std::string_view parse<StringLiteral>(Stream& stream) {
    if (!matches<StringLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), StringLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume through first "
    stream.consume_through('"');

    auto is_str_end = [prev_backslash = false](char c) mutable {
        if (prev_backslash) {
            prev_backslash = false;
            return false;
        }

        if (c == '\\') {
            prev_backslash = true;
        }

        return c == '"';
    };

    stream.consume_until(is_str_end);
    stream.consume(1);

    return get_res();
}

static_assert(test_parse<StringLiteral>(R"("")") == R"("")");
static_assert(test_parse<StringLiteral>(R"(u"a")") == R"(u"a")");
static_assert(test_parse<StringLiteral>(R"(u8"abc")") == R"(u8"abc")");
static_assert(test_parse<StringLiteral>(R"(L"abc  "  )") == R"(L"abc  ")");
static_assert(test_parse<StringLiteral>(R"("\"")") == R"("\"")");
static_assert(test_parse<StringLiteral>(R"("\""")") == R"("\"")");
static_assert(test_parse<StringLiteral>(R"("\"""")") == R"("\"")");
static_assert(test_parse<StringLiteral>(R"("\\"""")") == R"("\\")");
static_assert(test_parse<StringLiteral>(R"("\\\\\" \x12")") == R"("\\\\\" \x12")");

/// \overload
/// See \ref Token::Kind::RawStringLiteral for details
template <>
constexpr std::string_view parse<RawStringLiteral>(Stream& stream) {
    if (!matches<RawStringLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), RawStringLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume through first "
    stream.consume_until('"');
    stream.consume(1);

    // get d-char-seq
    std::string_view d_char_seq = stream.consume_until('(');

    // first, consume until the next ) char
    // then check if the d-char-seq AND a " follows that paren.
    //   if so -> we're done
    //   if not -> repeat checks above
    while (!stream.consume_through(')').empty() && !(stream.consume(d_char_seq) && stream.consume('"'))) {
    }

    return get_res();
}

static_assert(test_parse<RawStringLiteral>(R"---(R"()")---") == R"---(R"()")---");
static_assert(test_parse<RawStringLiteral>(R"---(LR"()")---") == R"---(LR"()")---");
static_assert(test_parse<RawStringLiteral>(R"---(u8R"()")---") == R"---(u8R"()")---");
static_assert(test_parse<RawStringLiteral>(R"---(R"(("))")---") == R"---(R"(("))")---");
static_assert(test_parse<RawStringLiteral>(R"---(R"a()a")---") == R"---(R"a()a")---");
static_assert(test_parse<RawStringLiteral>(R"---(R"a()a)a")---") == R"---(R"a()a)a")---");
static_assert(test_parse<RawStringLiteral>(R"---(R"123( 28%\di\""" 2)123")---") == R"---(R"123( 28%\di\""" 2)123")---");

/// \overload
/// See \ref Token::Kind::StringLiteral for details
template <>
constexpr std::string_view parse<CharLiteral>(Stream& stream) {
    if (!matches<CharLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), CharLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Exact same strategy used in parse<StringLiteral>, simply replacing " with '

    // Consume through first '
    stream.consume_through('\'');

    auto is_chr_end = [prev_backslash = false](char c) mutable {
        if (prev_backslash) {
            prev_backslash = false;
            return false;
        }

        if (c == '\\') {
            prev_backslash = true;
        }

        return c == '\'';
    };

    stream.consume_until(is_chr_end);
    stream.consume(1);

    return get_res();
}

static_assert(test_parse<CharLiteral>("''") == "''");
static_assert(test_parse<CharLiteral>("u''") == "u''");
static_assert(test_parse<CharLiteral>("u8''") == "u8''");
static_assert(test_parse<CharLiteral>("L''") == "L''");
static_assert(test_parse<CharLiteral>("'a'") == "'a'");
static_assert(test_parse<CharLiteral>(R"('\\')") == R"('\\')");
static_assert(test_parse<CharLiteral>(R"('\'')") == R"('\'')");
static_assert(test_parse<CharLiteral>("'abcd'") == "'abcd'");

constexpr bool consume_int_suffix(Stream& stream) {
    // an int suffix is 1-3 chars long, so just brute force
    for (std::size_t len = 3; len >= 1; --len) {
        if (len <= stream.size() && is_int_suffix(stream.peek(len))) {
            stream.consume(len);
            return true;
        }
    }

    return false;
}

/// \overload
/// See \ref Token::Kind::IntBinLiteral for details
template <>
constexpr std::string_view parse<IntBinLiteral>(Stream& stream) {
    if (!matches<IntBinLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), IntBinLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume the prefix
    if (!stream.consume("0b", case_insensitive)) {
        throw ParsingError("0b literal prefix missing", stream.str(), IntBinLiteral);
    }

    stream.consume_through(digit_or_sep);

    consume_int_suffix(stream);

    return get_res();
}

/// \overload
/// See \ref Token::Kind::IntOctLiteral for details
template <>
constexpr std::string_view parse<IntOctLiteral>(Stream& stream) {
    if (!matches<IntOctLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), IntOctLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume the prefix
    if (!stream.consume('0')) {
        throw ParsingError("0 literal prefix missing", stream.str(), IntOctLiteral);
    }

    stream.consume_through(digit_or_sep);

    consume_int_suffix(stream);

    return get_res();
}

/// \overload
/// See \ref Token::Kind::IntDecLiteral for details
template <>
constexpr std::string_view parse<IntDecLiteral>(Stream& stream) {
    if (!matches<IntDecLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), IntDecLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    stream.consume_through(digit_or_sep);

    consume_int_suffix(stream);

    return get_res();
}

/// \overload
/// See \ref Token::Kind::IntHexLiteral for details
template <>
constexpr std::string_view parse<IntHexLiteral>(Stream& stream) {
    if (!matches<IntHexLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), IntHexLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume the prefix
    if (!stream.consume("0x", case_insensitive)) {
        throw ParsingError("0x literal prefix missing", stream.str(), IntHexLiteral);
    }

    stream.consume_through(xdigit_or_sep);

    consume_int_suffix(stream);

    return get_res();
}

static_assert(test_parse<IntOctLiteral>("0") == "0");
static_assert(test_parse<IntOctLiteral>("0ull") == "0ull");
static_assert(test_parse<IntOctLiteral>("0U") == "0U");
static_assert(test_parse<IntOctLiteral>("0L") == "0L");
static_assert(test_parse<IntOctLiteral>("0UL") == "0UL");
static_assert(test_parse<IntDecLiteral>("123'456") == "123'456");
static_assert(test_parse<IntDecLiteral>("123+456") == "123");
static_assert(test_parse<IntDecLiteral>("123 + 456") == "123");
static_assert(test_parse<IntHexLiteral>("0x123ABC") == "0x123ABC");
static_assert(test_parse<IntBinLiteral>("0B01'01l") == "0B01'01l");
static_assert(test_parse<IntDecLiteral>("123_ab") == "123");

// testing for parsing of *really weird* (and UB) user-defined literal operators
static_assert(test_parse<IntDecLiteral>("123ABC") == "123");
static_assert(test_parse<IntDecLiteral>("1FFF") == "1");
static_assert(test_parse<IntDecLiteral>("123'123a_ab") == "123'123");
static_assert(test_parse<IntBinLiteral>("0b0101") == "0b0101");
static_assert(test_parse<IntBinLiteral>("0b0A123") == "0b0");

/// \overload
/// See \ref Token::Kind::FloatLiteral for details
template <>
constexpr std::string_view parse<FloatLiteral>(Stream& stream) {
    if (!matches<FloatLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), FloatLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    stream.consume_through(digit_or_sep);

    if (stream.consume('.')) {
        stream.consume_through(digit_or_sep);
    } else {
        // if there is no '.' then there must be an exponent
        if (tolower(stream.peek()) != 'e') {
            throw ParsingError("exponent char 'e'/'E' missing", stream.str(), FloatLiteral);
        }
    }

    // consume potential exponent and sign
    if (stream.consume('e', case_insensitive)) {
        stream.consume('+') || stream.consume('-');
    }

    stream.consume_through(digit_or_sep);

    // check for a floating point suffix
    std::string_view suffix = stream.peek_through(is_ident_like());

    if (suffix.size() == 1) {
        stream.consume('f', case_insensitive) || stream.consume('l', case_insensitive);
    }

    return get_res();
}

/// \overload
/// See \ref Token::Kind::FloatHexLiteral for details
template <>
constexpr std::string_view parse<FloatHexLiteral>(Stream& stream) {
    if (!matches<FloatHexLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), FloatHexLiteral);
    }

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Hex floating point literal may be of the following forms:
    //   hex-value hex-exp
    //   hex-value '.' [hex-value] hex-exp
    //   '.' hex-value hex-exp
    // In the following statement we ensure that it matches one of the above
    if (!stream.consume("0x", case_insensitive) && !stream.consume(".0x", case_insensitive)) {
        throw ParsingError("bad start of hex float literal", stream.str(), FloatHexLiteral);
    }

    stream.consume_through(xdigit_or_sep);

    // consume '.' if it exists
    stream.consume('.');

    // potentially consume the hex fractional part
    if (stream.consume("0x", case_insensitive)) {
        stream.consume_through(xdigit_or_sep);
    }

    // exponent is REQUIRED
    if (!stream.consume('p', case_insensitive)) {
        throw ParsingError("exponent char 'p'/'P' missing", stream.str(), FloatHexLiteral);
    }

    stream.consume('+') || stream.consume('-');

    stream.consume_through(digit_or_sep);

    // parse the potential floating-point-suffix outside of this lammbda

    // check for a floating point suffix
    std::string_view suffix = stream.peek_through(is_ident_like());

    if (suffix.size() == 1) {
        stream.consume('f', case_insensitive) || stream.consume('l', case_insensitive);
    }

    return get_res();
}

static_assert(test_parse<FloatLiteral>("0.") == "0.");
static_assert(test_parse<FloatLiteral>("123.") == "123.");
static_assert(test_parse<FloatLiteral>(".1") == ".1");
static_assert(test_parse<FloatLiteral>(".123") == ".123");
static_assert(test_parse<FloatLiteral>("0.0") == "0.0");
static_assert(test_parse<FloatLiteral>("123.456") == "123.456");
static_assert(test_parse<FloatLiteral>("1e123") == "1e123");
static_assert(test_parse<FloatLiteral>("1.e123") == "1.e123");
static_assert(test_parse<FloatLiteral>(".1e123") == ".1e123");
static_assert(test_parse<FloatLiteral>("1.1e123") == "1.1e123");
static_assert(test_parse<FloatLiteral>("1.1e+123") == "1.1e+123");
static_assert(test_parse<FloatLiteral>("1.1e-123") == "1.1e-123");
static_assert(test_parse<FloatLiteral>("1.1E123") == "1.1E123");
static_assert(test_parse<FloatLiteral>("1.1E+123") == "1.1E+123");
static_assert(test_parse<FloatLiteral>("1.1E-123") == "1.1E-123");
// test hex
static_assert(test_parse<FloatHexLiteral>("0x1p1") == "0x1p1");
static_assert(test_parse<FloatHexLiteral>("0x1p1") == "0x1p1");
static_assert(test_parse<FloatHexLiteral>("0x1.p1") == "0x1.p1");
static_assert(test_parse<FloatHexLiteral>(".0x1p1") == ".0x1p1");
static_assert(test_parse<FloatHexLiteral>(".0x1p1") == ".0x1p1");
static_assert(test_parse<FloatHexLiteral>("0x1.0x1p1") == "0x1.0x1p1");
static_assert(test_parse<FloatHexLiteral>("0xABCDEF1.0x1p1") == "0xABCDEF1.0x1p1");
static_assert(test_parse<FloatHexLiteral>("0Xabcdef1.0X1P1") == "0Xabcdef1.0X1P1");
static_assert(test_parse<FloatHexLiteral>("0x1.0xABCDEF1p1") == "0x1.0xABCDEF1p1");
static_assert(test_parse<FloatHexLiteral>("0x1.0Xabcdef1P1") == "0x1.0Xabcdef1P1");
static_assert(test_parse<FloatHexLiteral>("0xABCDEF1.0xABCDEF1p1") == "0xABCDEF1.0xABCDEF1p1");
static_assert(test_parse<FloatHexLiteral>("0Xabcdef1.0Xabcdef1P1") == "0Xabcdef1.0Xabcdef1P1");
static_assert(test_parse<FloatHexLiteral>("0x1p12345") == "0x1p12345");
static_assert(test_parse<FloatHexLiteral>("0x1p12345f") == "0x1p12345f");
static_assert(test_parse<FloatHexLiteral>("0x1p12345l") == "0x1p12345l");
static_assert(test_parse<FloatHexLiteral>("0x123.0xABCp10") == "0x123.0xABCp10");
// test (normal) suffixes
static_assert(test_parse<FloatLiteral>("1.f") == "1.f");
static_assert(test_parse<FloatLiteral>("1.l") == "1.l");
static_assert(test_parse<FloatLiteral>("1.0f") == "1.0f");
static_assert(test_parse<FloatLiteral>(".0f") == ".0f");
static_assert(test_parse<FloatLiteral>("123.0F") == "123.0F");
static_assert(test_parse<FloatLiteral>("123.0L") == "123.0L");
// test potential user-defined suffixes
static_assert(test_parse<FloatLiteral>("123.0labc") == "123.0");
static_assert(test_parse<FloatLiteral>("123.0f_abc") == "123.0");
static_assert(test_parse<FloatLiteral>("123e12fb") == "123e12");
// hex weirdness (though exponent is required, so there are a few less edge cases)
static_assert(test_parse<FloatLiteral>("123.0p") == "123.0");
static_assert(test_parse<FloatLiteral>("123.p") == "123.");
static_assert(test_parse<FloatHexLiteral>("0x123.p1e123") == "0x123.p1");
static_assert(test_parse<FloatHexLiteral>(".0x0p1f12") == ".0x0p1");
static_assert(test_parse<FloatHexLiteral>(".0x0p1f12") == ".0x0p1");
static_assert(test_parse<FloatHexLiteral>(".0x0p1l_12") == ".0x0p1");
static_assert(test_parse<FloatHexLiteral>(".0x0p1_12") == ".0x0p1");

/// \overload
/// See \ref Token::Kind::BoolLiteral for details
template <>
constexpr std::string_view parse<BoolLiteral>(Stream& stream) {
    if (!matches<BoolLiteral>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), BoolLiteral);
    }

    return stream.consume_through(is_ident_like());
}

/// \overload
/// See \ref Token::Kind::Identifier for details
template <>
constexpr std::string_view parse<Identifier>(Stream& stream) {
    if (!matches<Identifier>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), Identifier);
    }

    return stream.consume_through(is_ident_like());
}

static_assert(test_parse<Identifier>("abc") == "abc");
static_assert(test_parse<Identifier>("_") == "_");
static_assert(test_parse<Identifier>("_abc") == "_abc");
static_assert(test_parse<Identifier>("_12abc") == "_12abc");
static_assert(test_parse<Identifier>("_abc+2") == "_abc");
static_assert(test_parse<Identifier>("_abc-2") == "_abc");
static_assert(test_parse<Identifier>("_abc(2)") == "_abc");
static_assert(test_parse<Identifier>("a.b") == "a");

/// \overload
/// See \ref Token::Kind::Grouping for details
template <>
constexpr std::string_view parse<Grouping>(Stream& stream) {
    if (!matches<Grouping>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), Grouping);
    }

    // Raise a compilation error in case we ever change supported grouping tokens to multi-char
    static_assert(std::same_as<decltype(grouping_tokens)::value_type, char>);

    return stream.consume(1);
}

/// \overload
/// See \ref Token::Kind::BinaryOperator for details
template <>
constexpr std::string_view parse<BinaryOperator>(Stream& stream) {
    if (!matches<BinaryOperator>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), BinaryOperator);
    }

    // Same strategy as in matches<BinaryOperator>

    // To abide by the maximal munch rule, prioritizing bigger tokens over samller ones
    // e.g., "-=" is always picked over "-"
    auto max_munch_tokens = binary_operator_tokens;
    ranges::sort(max_munch_tokens, std::greater{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);
    const auto* iter = ranges::find_if(max_munch_tokens, check_stream_starts);

    if (iter == ranges::end(max_munch_tokens)) {
        throw ParsingError("no match found in binary operator token list", stream.str(), BinaryOperator);
    }

    return stream.consume(iter->size());
}

/// \overload
/// See \ref Token::Kind::Operator for details
template <>
constexpr std::string_view parse<Operator>(Stream& stream) {
    if (!matches<Operator>(stream)) {
        throw ParsingError("matches precondition failed in parse", stream.str(), Operator);
    }

    // Same logic as in parse<BinayOperator>
    // TODO: Make more DRY

    // To abide by the maximal munch rule, prioritizing bigger tokens over samller ones
    // e.g., "-=" is always picked over "-"
    auto max_munch_tokens = operator_tokens;
    ranges::sort(max_munch_tokens, std::greater{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);
    const auto* iter = ranges::find_if(max_munch_tokens, check_stream_starts);

    if (iter == ranges::end(max_munch_tokens)) {
        throw ParsingError("no match found in operator token list", stream.str(), Operator);
    }

    return stream.consume(iter->size());
}

/// Implementation for parsing the entirety of an input stream, accepting a list of parsable tokens as tparams
template <std::size_t MaxNumTokens, Token::Kind... ParsableTokenKinds>
constexpr auto parse_all(Stream input_stream) {
    std::array<Token, MaxNumTokens> tokens{};

    for (std::size_t i = 0; auto& tok : tokens) {
        // strip any leading whitespace (only ' ' and '\t')
        input_stream.consume_through(isblank);

        if (input_stream.empty()) {
            tok.kind = EndDelimiter;
            break;
        }

        // An expression of, uhh, questionable coding standards and cleanliness
        // It is very concise though.
        ((tok.kind = ParsableTokenKinds,
          matches<ParsableTokenKinds>(input_stream) && !(tok.str = parse<ParsableTokenKinds>(input_stream)).empty()) ||
         ...);

        input_stream.ctx.prevs = std::span(tokens.begin(), ++i);
    }

    if (!input_stream.empty()) {
        throw ParsingError("input stream non-empty after full parse", input_stream.str());
    }

    return tokens;
}

} // namespace tokenize

template <std::size_t MaxNumTokens>
constexpr auto parse_tokens(std::string_view str) {
    using enum Token::Kind;

    return tokenize::parse_all<MaxNumTokens, BoolLiteral, StringLiteral, RawStringLiteral, CharLiteral, IntBinLiteral,
                               IntOctLiteral, IntDecLiteral, IntHexLiteral, FloatLiteral, FloatHexLiteral, Identifier,
                               Grouping, BinaryOperator, Operator>(str);
}

template <std::size_t MaxNumTokens = 1'024>
class Tokenizer
{
public:
    constexpr Tokenizer() = default;

    constexpr explicit(false) Tokenizer(std::string_view str)
        : original_{str}
        , tokens_{parse_tokens<MaxNumTokens>(str)}
        , num_tokens_{find_end_delim_idx()} {}

    constexpr Tokenizer subseq(std::size_t start, std::size_t len) const {
        ASSERT(start < size());

        len = std::min(size() - start, len);

        Tokenizer result;

        ranges::copy(tokens_.begin() + start, tokens_.begin() + start + len, result.tokens_.begin());
        result.num_tokens_ = len;

        auto str_start = result.tokens_.front().str.data() - original_.data();
        auto str_len = (result[len - 1].str.data() + result[len - 1].str.size()) - result.tokens_.front().str.data();
        result.original_ = original_.substr(str_start, str_len);

        return result;
    }

    constexpr std::string_view get_original() const { return original_; }

    constexpr auto size() const { return num_tokens_; }

    constexpr auto empty() const { return num_tokens_ == 0; }

    constexpr auto begin() const { return tokens_.begin(); }

    constexpr auto end() const { return tokens_.begin() + num_tokens_; }

    constexpr bool operator==(const ranges::forward_range auto& other) const { return ranges::equal(*this, other); }

    constexpr const Token& operator[](std::size_t idx) const {
        ASSERT(idx < num_tokens_);
        return tokens_[idx];
    }

private:
    constexpr std::size_t find_end_delim_idx() const {
        auto iter =
            std::ranges::find_if(tokens_, [](const Token& tok) { return tok.kind == Token::Kind::EndDelimiter; });
        if (iter == ranges::end(tokens_)) {
            throw ParsingError("EndDelimiter missing from token stream");
        }

        return gsl::narrow_cast<std::size_t>(iter - ranges::begin(tokens_));
    }

    std::string_view original_;
    std::array<Token, MaxNumTokens> tokens_;
    std::size_t num_tokens_{};
};

} // namespace asmgrader::inspection
