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
#include <range/v3/algorithm/count.hpp>
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
#include <functional>
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
        IntLiteral,

        /// https://en.cppreference.com/w/cpp/language/floating_literal.html
        ///
        /// FloatingPointLiteral = ( dec-value | hex-val ) floating-point-suffix
        ///
        /// dec-value = dec-digits dec-exp
        ///           | dec-digits '.' [ dec-exp ]
        ///           | [ dec-digits ] '.' dec-digits [ dec-exp ]
        /// hex-value = '0x'i hex-val-nopre
        /// hex-val-nopre = hex-digits hex-exp
        ///               | hex-digits '.' hex-exp
        ///               | [ hex-digits ] '.' hex-digits hex-exp
        ///
        /// dec-digits = ( '1'..'9' ) { '0'..'9'  | DIGIT_SEP }
        /// dec-exp = 'e'i [ SIGN ] dec-seq
        /// hex-digits = ( '0'..'9' | 'a'i..'f'i ) { '0'..'9' | 'a'i..'f'i | DIGIT_SEP }
        /// hex-exp = 'p'i [ SIGN ] dec-seq
        ///
        /// SIGN = '+' | '-'
        ///
        /// floating-point-suffix = 'f'i | 'l'i
        FloatingPointLiteral,

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
        /// Imperatively defined any of the symbols in the list below when they are not part of a previously defined
        /// token and do not meet the requirements for \ref Grouping. Perhaps a little confusingly, since the token
        /// stream is flat, operators like `a[]` produce 2 seperate operator tokens of '[' and ']'.
        ///   '::'
        ///   '++', '--'                                                    *** no distinction between pre and post
        ///   '(', ')', '[', ']', '.', '->'
        ///   '.*', '->*'
        ///   '+', '-', '*', '/', '%',
        ///   '<<', ">>', '^', '|', '&', '~'
        ///   '&&', '||'
        ///   '!', '==', '!=', '<=>', '<', '<=', '>', '>='
        ///   'throw', 'sizeof', 'alignof', 'new', 'delete',
        ///   'const_cast', 'static_cast', 'dynamic_cast', 'reinterpret_cast',
        ///   '=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=',
        ///   ',', '?', ':'
        ///   also includes literal operators
        /// TODO: Maybe support alternate spellings like 'and', 'not', etc.
        Operator,

        /// Deliminates the end of the token sequence.
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
    case IntLiteral:
        return "IntLiteral";
    case FloatingPointLiteral:
        return "FloatingPointLiteral";
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

inline std::string format_as(const Token& tok) {
    return fmt::format("{:?}:{}", tok.str, tok.kind);
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

    constexpr explicit(false) Stream(std::string_view& string)
        : str_{string} {}

    /// This overload is just used for writing tests
    constexpr Stream(std::string_view& string, StreamContext context)
        : ctx{context}
        , str_{string} {}

    constexpr std::size_t size() const { return str().size(); }

    constexpr bool empty() const { return str().empty(); }

    /// Peek at the first character of the stream
    constexpr char peek() const {
        ASSERT(!str().empty());
        return str().at(0);
    }

    /// \overload
    /// Peek at the first n characters of the stream
    constexpr std::string_view peek(std::size_t n) const {
        ASSERT(n <= str().size());
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

    constexpr bool consume(std::string_view str, CaseInsensitiveTag /*unused*/) {
        using std::size;
        const auto str_sz = std::string_view{str}.size();

        if (this->size() < str_sz) {
            return false;
        }

        // Not sure why '| ranges::views::transform(tolower)' is able to be used constexpr here...
        if (!ranges::equal(peek(str_sz), str, std::equal_to{}, tolower, tolower)) {
            return false;
        }
        idx_ += str_sz;

        return true;
    }

    constexpr bool consume(char chr, CaseInsensitiveTag /*unused*/) {
        if (size() == 0 || tolower(peek()) != tolower(chr)) {
            return false;
        }

        idx_ += 1;

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

    constexpr std::string_view str() const { return str_.substr(idx_); }

private:
    std::string_view str_;
    std::size_t idx_ = 0;
};

constexpr auto operator_tokens = std::to_array<std::string_view>({
    "::",                                                              //
    "++", "--",                                                        //
    "(", ")", "[", "]", ".", "->",                                     //
    ".*", "->*",                                                       //
    "+", "-", "*", "/", "%",                                           //
    "<<", ">>", "^", "|", "&", "~",                                    //
    "&&", "||",                                                        //
    "!", "==", "!=", "<=>", "<", "<=", ">", ">=",                      //
    "throw", "sizeof", "alignof", "new", "delete",                     //
                                                                       //
    "const_cast", "static_cast", "dynamic_cast", "reinterpret_cast",   //
    "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", //
    ",", "?", ":"                                                      //
});

constexpr auto grouping_tokens = std::to_array<char>({
    '{',
    '}', //
    '(',
    ')', //
    '<',
    '>', //
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
/// Whether the start of `stream` is a integer literal token
/// See \ref Token::Kind::IntLiteral for details
template <>
constexpr bool matches<IntLiteral>(const Stream& stream) {
    if (!isdigit(stream.peek())) {
        return false;
    }

    // now need to do a few checks to make sure it's not a floating point literal
    // we'll reuse this logic in `matches<FloatingPointLiteral>`

    auto or_sep = [](std::invocable<char> auto digit_check, char c) { return digit_check(c) || c == '\''; };

    // sep is not allowed at the front, but we already checked that the first char is a digit
    // also, 12''34 is not a valid literal, but c++ forbids that anyways
    std::string_view after_digits = stream.peek_past(std::bind_front(or_sep, isdigit));

    // int literal is at the end of the stream
    if (after_digits.empty()) {
        return true;
    }

    if (char first = tolower(after_digits.front())) {
        if (first == 'b') {
            return true;
        }

        // floating point literal
        if (first == 'e' || first == '.') {
            return false;
        }

        // Not a hex literal and not a floating point -> must be a literal suffix
        if (first != 'x') {
            return true;
        }
    }

    // At this point, we must have 0x...
    //                  we're here  ^

    // fully remove int/floating literal prefix 0x
    after_digits.remove_prefix(1);

    // obtain portion after all hex digits
    after_digits = substr_past(after_digits, std::bind_front(or_sep, isxdigit));

    // after_digits is empty -> hex int literal at end of stream -> GOOD
    // after_digits first char is 'p' (floating hex literal exponent) or '.' floating poing -> BAD
    return after_digits.empty() || (after_digits.front() != 'p' && after_digits.front() != '.');
}

static_assert(matches<IntLiteral>("123"));
static_assert(matches<IntLiteral>("123+"));
static_assert(matches<IntLiteral>("123u"));
static_assert(matches<IntLiteral>("0xAEF102"));
static_assert(matches<IntLiteral>("0xAEF102ull"));
static_assert(matches<IntLiteral>("0b1010"));
static_assert(matches<IntLiteral>("0"));
static_assert(matches<IntLiteral>("01'2"));
static_assert(matches<IntLiteral>("0'1'2345'123"));
static_assert(matches<IntLiteral>("0x1'A'b3F5'123"));
static_assert(!matches<IntLiteral>("'0"));
static_assert(!matches<IntLiteral>("0.123"));
static_assert(!matches<IntLiteral>("10.123"));
static_assert(!matches<IntLiteral>(".123"));
static_assert(!matches<IntLiteral>("0.f"));
static_assert(!matches<IntLiteral>("1e5"));
static_assert(!matches<IntLiteral>("0x15p3"));

/// \overload
/// Whether the start of `stream` is a floating point literal token
/// See \ref Token::Kind::FloatingPointLiteral for details
template <>
constexpr bool matches<FloatingPointLiteral>(const Stream& stream) {
    // Starts with digit and is NOT an IntLiteral -> must be a FloatingPointLiteral
    if (isdigit(stream.peek())) {
        return !matches<IntLiteral>(stream);
    }

    // either operator. or a fp number
    if (stream.peek() == '.') {
        char after_dec = stream.peek(2).back();
        // a digit after the . means it cannot be an operator
        return isdigit(after_dec);
    }

    return false;
}

static_assert(matches<FloatingPointLiteral>(".123"));
static_assert(matches<FloatingPointLiteral>(".123e52"));
static_assert(matches<FloatingPointLiteral>(".123e+52"));
static_assert(matches<FloatingPointLiteral>(".123e-52"));
static_assert(matches<FloatingPointLiteral>("0.123"));
static_assert(matches<FloatingPointLiteral>("10.123e31"));
static_assert(matches<FloatingPointLiteral>("12e42"));
static_assert(matches<FloatingPointLiteral>("10.123f"));
static_assert(matches<FloatingPointLiteral>("0xAEFp3"));
static_assert(matches<FloatingPointLiteral>("0xAEFp+3"));
static_assert(matches<FloatingPointLiteral>("0xAEFp-3"));
static_assert(matches<FloatingPointLiteral>("0xAEF.0x123p3"));
static_assert(matches<FloatingPointLiteral>(".0x123p3"));
static_assert(matches<FloatingPointLiteral>("0.fl"));
static_assert(matches<FloatingPointLiteral>("0'123.1'2345'6fl"));
static_assert(matches<FloatingPointLiteral>("0x12'1EF.p0x123"));
static_assert(matches<FloatingPointLiteral>(".0FL"));
static_assert(!matches<FloatingPointLiteral>("123"));
static_assert(!matches<FloatingPointLiteral>("0"));
static_assert(!matches<FloatingPointLiteral>("0b10"));
static_assert(!matches<FloatingPointLiteral>("0xAB10"));

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

    return !ranges::contains(operator_tokens, full_token);
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
        ASSERT(num_operator_closing <= num_operator_opening);

        // no unmatched operator opening. We must be matching to the previous grouping symbol
        if (num_operator_opening == num_operator_closing) {
            return true;
        }

        auto last_operator_opening = ranges::find(stream.ctx.prevs, Token{.kind = Operator, .str = "("});

        return last_grouping_opening > last_operator_opening;
    }

    // very basic heuristic for checking if '<' / '>' are being used to surround tparams:
    //   for '<': if an excess '>' is found further in the stream, before any other '<' chars, there is no logical
    //   operator in-between, and there is no opening parenthesis in-between
    //     (just includes '&&', '||')
    //   for '>': an unmatched '<' exists

    // FIXME: This is not worth implementing properly right now
    //        as long as expressions like the following forms can be parsed:
    //        `ident<simple-expr>`
    //        `ident<simple-expr> </> ident<simple-expr>`
    //        `ident<simple-expr> </> ident`
    //        `ident </> ident<simple-expr>`
    // TODO: handle bitshift ambiguity and '<' / '>' operators nested within a tparam
    if (tok == '<') {
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
/// Whether the start of `stream` is an identifier token
/// See \ref Token::Kind::Identifier for details
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
    // FIXME:...
    // ranges::sort(max_munch_tokens, std::less{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);
    return ranges::any_of(max_munch_tokens, check_stream_starts);
}

static_assert(matches<Operator>("+ 123"));
static_assert(matches<Operator>(":: 123"));
static_assert(matches<Operator>("? 123"));
static_assert(!matches<Operator>("(123)"));
static_assert(!matches<Operator>("{ 123"));

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
    DEBUG_ASSERT(matches<StringLiteral>(stream));

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
    DEBUG_ASSERT(matches<RawStringLiteral>(stream));

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
    DEBUG_ASSERT(matches<CharLiteral>(stream));

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

/// \overload
/// See \ref Token::Kind::IntLiteral for details
template <>
constexpr std::string_view parse<IntLiteral>(Stream& stream) {
    DEBUG_ASSERT(matches<IntLiteral>(stream));

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    // Consume the first digit
    stream.consume(1);

    if (stream.empty()) {
        return get_res();
    }

    // std::function is not allowed in constexpr pre-C++23 :(
    bool (*digit_or_sep)(char){};

    if (tolower(stream.peek()) == 'x') {
        if (stream.size() == 1 || !isxdigit(stream.peek(2).at(1))) {
            return get_res();
        }
        stream.consume(1);
        digit_or_sep = [](char c) { return isxdigit(c) || c == '\''; };
    } else {
        // we don't want to attempt to consume hex digits for non-hex literals,
        // since they can *technically* be a user-defined literal operator
        //   e.g., "01abc"
        // Attempting to consume normal decimal degits for a binary literal is
        // fine, since those could never be a literal operator.
        digit_or_sep = [](char c) { return isdigit(c) || c == '\''; };
    }

    if (tolower(stream.peek()) == 'b') {
        if (stream.size() == 1 || !isdigit(stream.peek(2).at(1))) {
            return get_res();
        }
        stream.consume(1);
    }

    // consumes all digits
    stream.consume_while(digit_or_sep);

    // an int suffix is 1-3 chars long, so just brute force
    for (std::size_t len = 3; len >= 1; --len) {
        if (len <= stream.size() && is_int_suffix(stream.peek(len))) {
            stream.consume(len);
            break;
        }
    }

    return get_res();
}

static_assert(test_parse<IntLiteral>("0") == "0");
static_assert(test_parse<IntLiteral>("0ull") == "0ull");
static_assert(test_parse<IntLiteral>("0U") == "0U");
static_assert(test_parse<IntLiteral>("0L") == "0L");
static_assert(test_parse<IntLiteral>("0UL") == "0UL");
static_assert(test_parse<IntLiteral>("123'456") == "123'456");
static_assert(test_parse<IntLiteral>("123+456") == "123");
static_assert(test_parse<IntLiteral>("123 + 456") == "123");
static_assert(test_parse<IntLiteral>("0x123ABC") == "0x123ABC");
static_assert(test_parse<IntLiteral>("0B01'01l") == "0B01'01l");
static_assert(test_parse<IntLiteral>("123_ab") == "123");

// testing for parsing of *really weird* (and UB) user-defined literal operators
static_assert(test_parse<IntLiteral>("1x") == "1");
static_assert(test_parse<IntLiteral>("1b") == "1");
static_assert(test_parse<IntLiteral>("123ABC") == "123");
static_assert(test_parse<IntLiteral>("1FFF") == "1");
static_assert(test_parse<IntLiteral>("123'123a_ab") == "123'123");
static_assert(test_parse<IntLiteral>("0b0101") == "0b0101");
static_assert(test_parse<IntLiteral>("0b0A123") == "0b0");

/// \overload
/// See \ref Token::Kind::FloatingPointLiteral for details
template <>
constexpr std::string_view parse<FloatingPointLiteral>(Stream& stream) {
    DEBUG_ASSERT(matches<FloatingPointLiteral>(stream));

    auto get_res = [init = stream.str(), &stream] { return init.substr(0, init.size() - stream.str().size()); };

    auto xdigit_or_sep = [](char c) { return isxdigit(c) || c == '\''; };
    auto digit_or_sep = [](char c) { return isdigit(c) || c == '\''; };

    auto try_parse_hex = [&]() -> bool {
        // Hex floating point literal may be of the following forms:
        //   hex-value hex-exp
        //   hex-value '.' [hex-value] hex-exp
        //   '.' hex-value hex-exp
        // In the following statement we check that it matches one of the above
        if (!stream.consume("0x", case_insensitive) && !stream.consume(".0x", case_insensitive)) {
            return false;
        }
        // A note on the above:
        //   technically, we could have '0xabc' where 'xabc' is a user-defined literal operator
        //   (this would be UB, but well-formed syntactically). However, in this function's case,
        //   we assume that the value MUST BE floating point, thus a literal suffix cannot occur here.

        stream.consume_through(xdigit_or_sep);

        // consume '.' if it exists
        stream.consume('.');

        // potentially consume the hex fractional part
        if (stream.consume("0x", case_insensitive)) {
            stream.consume_through(xdigit_or_sep);
        }

        // exponent is REQUIRED
        ASSERT(stream.consume('p', case_insensitive), stream.str());

        stream.consume('+') || stream.consume('-');

        stream.consume_through(digit_or_sep);

        // parse the potential floating-point-suffix outside of this lammbda

        return true;
    };

    if (!try_parse_hex()) {
        stream.consume_through(digit_or_sep);

        if (stream.consume('.')) {
            stream.consume_through(digit_or_sep);
        } else {
            // if there is no '.' then there must be an exponent
            ASSERT(tolower(stream.peek()) == 'e', stream.str());
        }

        // consume potential exponent and sign
        if (stream.consume('e', case_insensitive)) {
            stream.consume('+') || stream.consume('-');
        }

        stream.consume_through(digit_or_sep);
    }

    // check for a floating point suffix
    std::string_view suffix = stream.peek_through(is_ident_like());

    if (suffix.size() == 1) {
        stream.consume('f', case_insensitive) || stream.consume('l', case_insensitive);
    }

    return get_res();
}

static_assert(test_parse<FloatingPointLiteral>("0.") == "0.");
static_assert(test_parse<FloatingPointLiteral>("123.") == "123.");
static_assert(test_parse<FloatingPointLiteral>(".1") == ".1");
static_assert(test_parse<FloatingPointLiteral>(".123") == ".123");
static_assert(test_parse<FloatingPointLiteral>("0.0") == "0.0");
static_assert(test_parse<FloatingPointLiteral>("123.456") == "123.456");
static_assert(test_parse<FloatingPointLiteral>("1e123") == "1e123");
static_assert(test_parse<FloatingPointLiteral>("1.e123") == "1.e123");
static_assert(test_parse<FloatingPointLiteral>(".1e123") == ".1e123");
static_assert(test_parse<FloatingPointLiteral>("1.1e123") == "1.1e123");
static_assert(test_parse<FloatingPointLiteral>("1.1e+123") == "1.1e+123");
static_assert(test_parse<FloatingPointLiteral>("1.1e-123") == "1.1e-123");
static_assert(test_parse<FloatingPointLiteral>("1.1E123") == "1.1E123");
static_assert(test_parse<FloatingPointLiteral>("1.1E+123") == "1.1E+123");
static_assert(test_parse<FloatingPointLiteral>("1.1E-123") == "1.1E-123");
// test hex
static_assert(test_parse<FloatingPointLiteral>("0x1p1") == "0x1p1");
static_assert(test_parse<FloatingPointLiteral>("0x1p1") == "0x1p1");
static_assert(test_parse<FloatingPointLiteral>("0x1.p1") == "0x1.p1");
static_assert(test_parse<FloatingPointLiteral>(".0x1p1") == ".0x1p1");
static_assert(test_parse<FloatingPointLiteral>(".0x1p1") == ".0x1p1");
static_assert(test_parse<FloatingPointLiteral>("0x1.0x1p1") == "0x1.0x1p1");
static_assert(test_parse<FloatingPointLiteral>("0xABCDEF1.0x1p1") == "0xABCDEF1.0x1p1");
static_assert(test_parse<FloatingPointLiteral>("0Xabcdef1.0X1P1") == "0Xabcdef1.0X1P1");
static_assert(test_parse<FloatingPointLiteral>("0x1.0xABCDEF1p1") == "0x1.0xABCDEF1p1");
static_assert(test_parse<FloatingPointLiteral>("0x1.0Xabcdef1P1") == "0x1.0Xabcdef1P1");
static_assert(test_parse<FloatingPointLiteral>("0xABCDEF1.0xABCDEF1p1") == "0xABCDEF1.0xABCDEF1p1");
static_assert(test_parse<FloatingPointLiteral>("0Xabcdef1.0Xabcdef1P1") == "0Xabcdef1.0Xabcdef1P1");
static_assert(test_parse<FloatingPointLiteral>("0x1p12345") == "0x1p12345");
static_assert(test_parse<FloatingPointLiteral>("0x1p12345f") == "0x1p12345f");
static_assert(test_parse<FloatingPointLiteral>("0x1p12345l") == "0x1p12345l");
// test (normal) suffixes
static_assert(test_parse<FloatingPointLiteral>("1.f") == "1.f");
static_assert(test_parse<FloatingPointLiteral>("1.l") == "1.l");
static_assert(test_parse<FloatingPointLiteral>("1.0f") == "1.0f");
static_assert(test_parse<FloatingPointLiteral>(".0f") == ".0f");
static_assert(test_parse<FloatingPointLiteral>("123.0F") == "123.0F");
static_assert(test_parse<FloatingPointLiteral>("123.0L") == "123.0L");
// test potential user-defined suffixes
static_assert(test_parse<FloatingPointLiteral>("123.0labc") == "123.0");
static_assert(test_parse<FloatingPointLiteral>("123.0f_abc") == "123.0");
static_assert(test_parse<FloatingPointLiteral>("123e12fb") == "123e12");
// hex weirdness (though exponent is required, so there are a few less edge cases)
static_assert(test_parse<FloatingPointLiteral>("123.0p") == "123.0");
static_assert(test_parse<FloatingPointLiteral>("123.p") == "123.");
static_assert(test_parse<FloatingPointLiteral>("0x123.p1e123") == "0x123.p1");
static_assert(test_parse<FloatingPointLiteral>(".0x0p1f12") == ".0x0p1");
static_assert(test_parse<FloatingPointLiteral>(".0x0p1f12") == ".0x0p1");
static_assert(test_parse<FloatingPointLiteral>(".0x0p1l_12") == ".0x0p1");
static_assert(test_parse<FloatingPointLiteral>(".0x0p1_12") == ".0x0p1");

/// \overload
/// See \ref Token::Kind::Identifier for details
template <>
constexpr std::string_view parse<Identifier>(Stream& stream) {
    DEBUG_ASSERT(matches<Identifier>(stream));

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
    DEBUG_ASSERT(matches<Grouping>(stream));

    // Raise a compilation error in case we ever change supported grouping tokens to multi-char
    static_assert(std::same_as<decltype(grouping_tokens)::value_type, char>);

    return stream.consume(1);
}

// TODO: tests

/// \overload
/// See \ref Token::Kind::Operator for details
template <>
constexpr std::string_view parse<Operator>(Stream& stream) {
    DEBUG_ASSERT(matches<Operator>(stream));

    // Same strategy as in matches<Operator>

    // To abide by the maximal munch rule, prioritizing bigger tokens over samller ones
    // e.g., "-=" is always picked over "-"
    auto max_munch_tokens = operator_tokens;
    // FIXME:...
    // ranges::sort(max_munch_tokens, std::less{}, &std::string_view::size);

    auto check_stream_starts = std::bind_front(&Stream::starts_with<std::string_view>, stream);
    const auto* iter = ranges::find_if(max_munch_tokens, check_stream_starts);

    ASSERT(iter != ranges::end(max_munch_tokens));

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

    ASSERT(input_stream.empty(), input_stream, tokens);

    return tokens;
}

} // namespace tokenize

template <std::size_t MaxNumTokens>
constexpr auto parse_tokens(std::string_view str) {
    using enum Token::Kind;

    return tokenize::parse_all<MaxNumTokens, StringLiteral, RawStringLiteral, CharLiteral, IntLiteral,
                               FloatingPointLiteral, Identifier, Grouping, Operator>(str);
}

template <std::size_t MaxNumTokens = 1'024>
class Tokenizer
{
public:
    constexpr explicit(false) Tokenizer(std::string_view str)
        : tokens_{parse_tokens<MaxNumTokens>(str)}
        , num_tokens_{find_end_delim_idx()} {}

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
        ASSERT(iter != ranges::end(tokens_), tokens_);

        return gsl::narrow_cast<std::size_t>(iter - ranges::begin(tokens_));
    }

    std::array<Token, MaxNumTokens> tokens_;
    std::size_t num_tokens_;
};

} // namespace asmgrader::inspection
