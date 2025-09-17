/// \file Implentation of inspection facilities for simple C++ expressions
/// Intended for usage with \ref `REQUIRE`
///
/// The ideas are heavily inspired by the libassert library, but the implementation is fully my own.
/// Why do this when an implementation already exists? Mostly cause parsing is kind of fun, and for a bit of learning.
#pragma once

#include <asmgrader/common/cconstexpr.hpp>

#include <libassert/assert.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/find_if_not.hpp>
#include <range/v3/range/concepts.hpp>

#include <array>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
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

        /// https://en.cppreference.com/w/cpp/language/operator_precedence.html
        /// (Note that contrary to the title of this link, this impl has no concept of operator precedence)
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
        ///   'throw', 'sizeof', 'new', 'delete',
        ///   '=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=',
        ///   ',', '?', ':'
        Operator,

        /// Imperatively defined as:
        /// '{', '}'
        /// '(', ')' - when not as a function call
        /// '<', '>' - in template context
        Grouping,

        /// Deliminates the end of the token sequence.
        /// With the current implementation, this is *never* accessible by the user.
        EndDelimiter
    };

    Kind kind;
    std::string_view str;

    static constexpr auto parse(std::string_view str);
};

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
/// or the entirety of `str` if `pred` is never satisfied found
[[nodiscard]] constexpr std::string_view substr_to(std::string_view str, std::invocable<char> auto pred) {
    auto iter = ranges::find_if(str, pred);

    if (iter == str.end()) {
        return str;
    }

    return str.substr(0, static_cast<std::size_t>(iter - str.begin()));
}

static_assert(substr_to("abc0123 ab", isdigit) == "abc");
static_assert(substr_to("0123 ab", std::not_fn(isdigit)) == "0123");

///////////////// character stream structure

struct Stream : std::string_view
{
    using std::string_view::string_view;

    // Peek at the first character of the stream
    constexpr char peek() const { return at(0); }

    // \overload
    // Peek at the first n characters of the stream
    constexpr std::string_view peek(std::size_t n) { return substr(0, n); }

    // \ref substr_to
    constexpr std::string_view peek_until(auto what) { return substr_to(*this, what); }

    // Peek past the first occurrence of `what` as found by \ref peek_until
    constexpr std::string_view peek_past(auto what) {
        auto skip_len = peek_until(what).size();

        return substr(skip_len);
    }

    // Peek past the first occurrence of `what` as found by \ref peek_until
    constexpr std::string_view peek_past(std::invocable<char> auto what) {
        auto skip_len = peek_until(std::not_fn(what)).size();

        return substr(skip_len);
    }
};

constexpr auto operator_tokens = std::to_array<std::string_view>({
    "::",                                                                           //
    "++",    "--",                                                                  //
    "(",     ")",      "[",   "]",      ".",  "->",                                 //
    ".*",    "->*",                                                                 //
    "+",     "-",      "*",   "/",      "%",                                        //
    "<<",    ">>",     "^",   "|",      "&",  "~",                                  //
    "&&",    "||",                                                                  //
    "!",     "==",     "!=",  "<=>",    "<",  "<=", ">",   ">=",                    //
    "throw", "sizeof", "new", "delete",                                             //
    "=",     "+=",     "-=",  "*=",     "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", //
    ",",     "?",      ":"                                                          //
});

constexpr auto grouping_tokens = std::to_array<char>({
    '{', '}', //
    '(', ')', //
    '<', '>', //
});

using enum Token::Kind;

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
    if (str.empty() || str.size() > 2) {
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

    if (tolower(str.front()) == 'l') {
        return str.empty();
    }

    return false;
}

static_assert(is_strlike_prefix("L") && is_strlike_prefix("u") && is_strlike_prefix("U8"));
static_assert(!is_strlike_prefix("au8") && !is_strlike_prefix("\"L"));

/// Check whether the start of `stream` matches a token kind
/// Assumes that there is no leading whitespace in `stream`
///
/// \param stream The character stream
/// \tparam Kind The kind of token to match to
template <Token::Kind Kind>
constexpr bool matches(Stream stream) {
    ASSERT(!stream.empty());

    return false;
}

/// Whether the start of `stream` is a string literal token
/// See \ref Token::Kind::StringLiteral for details
template <>
constexpr bool matches<StringLiteral>(Stream stream) {
    ASSERT(!stream.empty());

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

/// Whether the start of `stream` is a raw string literal token
/// See \ref Token::Kind::RawStringLiteral for details
template <>
constexpr bool matches<RawStringLiteral>(Stream stream) {
    ASSERT(!stream.empty());

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

/// Whether the start of `stream` is a char literal token
/// See \ref Token::Kind::CharLiteral for details
template <>
constexpr bool matches<CharLiteral>(Stream stream) {
    ASSERT(!stream.empty());

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

/// Whether the start of `stream` is a integer literal token
/// See \ref Token::Kind::IntLiteral for details
template <>
constexpr bool matches<IntLiteral>(Stream stream) {
    ASSERT(!stream.empty());

    if (!isdigit(stream.front())) {
        return false;
    }

    // now need to do a few checks to make sure it's not a floating point literal
    // we'll reuse this logic in `matches<FloatingPointLiteral>`

    std::string_view after_digits = stream.peek_past(isdigit);

    // int literal is at the end of the stream
    if (after_digits.empty()) {
        return true;
    }

    // check if str of contiguous alpha chars is an int suffix
    // "123u" takes "u"
    // "123u + 123" takes "u"
    // "123ul + 123" takes "u"
    if (is_int_suffix(substr_to(after_digits, std::not_fn(isalpha)))) {
        return true;
    }

    if (char first = tolower(after_digits.front())) {
        if (first == 'b') {
            return true;
        }

        // not a valid int literal prefix (0x | 0b)
        // could be 'e' or '.' for floating point exponent, etc.
        if (first != 'x') {
            return false;
        }
    }

    // fully remove int/floating literal prefix 0x
    after_digits.remove_prefix(1);

    // obtain portion after all hex digits
    ASSERT(after_digits.size() == 3);
    after_digits = substr_to(after_digits, std::not_fn(isxdigit));
    ASSERT(after_digits.size() == 3);

    // (same as earlier)
    // check if str of contiguous alpha chars is an int suffix
    //
    // if false, then it must be a floating point literal
    // could have been 'p' or '.' after hex value literal
    return after_digits.empty() || is_int_suffix(substr_to(after_digits, std::not_fn(isalpha)));
}

static_assert(matches<IntLiteral>("123"));
static_assert(matches<IntLiteral>("0xAEF"));
static_assert(matches<IntLiteral>("0b1010"));
static_assert(!matches<IntLiteral>("0.123"));
static_assert(!matches<IntLiteral>("10.123"));
static_assert(!matches<IntLiteral>(".123"));
static_assert(!matches<IntLiteral>("0f"));

/// Whether the start of `stream` is a floating point literal token
/// See \ref Token::Kind::FloatingPointLiteral for details
template <>
constexpr bool matches<FloatingPointLiteral>(Stream stream) {
    ASSERT(!stream.empty());

    if (isdigit(stream.peek())) {
        return !matches<IntLiteral>(stream);
    }

    // either operator. or a fp number
    if (stream.peek() == '.') {
        // a digit after the . means it cannot be an operator
        return isdigit(stream.peek(2).back());
    }

    return false;
}

// TODO: hex literal tests
static_assert(matches<FloatingPointLiteral>(".123"));
static_assert(matches<FloatingPointLiteral>("0.123"));
static_assert(matches<FloatingPointLiteral>("10.123"));
static_assert(matches<FloatingPointLiteral>("0xAEF"));
static_assert(matches<FloatingPointLiteral>("0f"));
static_assert(matches<FloatingPointLiteral>("0F"));

/// Returns which kind the *first* token found in `str` is
// TODO: No
constexpr Token::Kind which_kind(std::string_view stream) {
    using enum Token::Kind;

    // empty str -> end of token stream
    if (stream.empty()) {
        return EndDelimiter;
    }

    return Unknown;
}

// FIXME: NO no NO
constexpr ranges::range auto parse_tokens(std::string_view raw_str) {
    using enum Token::Kind;

    namespace views = ranges::views;

    constexpr auto take_remove_while = [](std::string_view& str, std::invocable<char> auto& func,
                                          std::size_t start_at = 0) {
        auto iter = ranges::find_if_not(str.begin() + start_at, str.end(), func);

        std::size_t len = iter - str.begin();

        std::string_view res = str.substr(0, len);
        str.remove_prefix(len);

        return res;
    };

    // for string OR character literals, since the logic is almost the same
    constexpr auto is_strlike_end = [](char check) {
        return [check, prev = '\0'](char c) mutable { return (c == check && std::exchange(prev, c) != '\\'); };
    };

    constexpr auto is_rstr_end = [](std::string_view d_char_seq) {
        return [need_seq = d_char_seq, found_paren = false, d_char_seq](char c) mutable {
            if (!found_paren) {
                found_paren = (c == ')');
                return true;
            }

            // We've matched the l-paren and all of d_char_seq; if c == '"' then we're done
            if (need_seq.empty()) {
                found_paren = false;
                need_seq = d_char_seq;
                return c != '"';
            }

            if (c == need_seq.front()) {
                need_seq.remove_prefix(1);
            } else {
                found_paren = false;
                need_seq = d_char_seq;
            }

            return true;
        };
    };

    constexpr auto valid_ident_chr = [](char c) { return isalpha(c) || isdigit(c) || c == '_'; };

    constexpr auto get_next_token = [&take_remove_while, &is_strlike_end](std::string_view& remaining_str) -> Token {
        // We assume that parsing thus far has been successful to keep very this simple
        // Also can of course assume valid C++ syntax

        const auto end = remaining_str.end();

        // remove leading whitespace
        std::ignore = take_remove_while(remaining_str, isspace);

        if (remaining_str.empty()) {
            return {.kind = EndDelimiter, .str = {}};
        }

        bool is_string_literal = false;
        bool is_char_literal = false;

        // starts with alpha character => Either an identifier or prefix to a string/character literal
        // we are just checking if it's any kind of string or character literal here
        // e.g., R"(abcd)", u"I'm utf-8! Probably will never be used D:", etc.
        // See: https://en.cppreference.com/w/cpp/language/string_literal.html
        if (isalpha(remaining_str.front())) {
            // look for the first non-alphanumeric character.
            // If this is a '"', then we can assume that it's a string literal
            // If it's not valid, the compiler is going to yell at you anyways
            auto iter = ranges::find_if_not(remaining_str, isalpha);

            if (iter == end) {
                std::string_view ident_res = remaining_str;
                remaining_str = {}; // clear remaining view (we're using the rest)
                return {.kind = Identifier, .str = remaining_str};
            }

            is_string_literal = (*iter == '"');
            is_char_literal = (*iter == '\'');
        }

        if (is_char_literal) {
            auto char_literal_start = remaining_str.find('\'');
            ASSERT(char_literal_start != std::string_view::npos);

            // take chars UNTIL (i.e., "while not") the end of the char
            // Starts after the first ' to not get a false positive
            // If it's some invalid multi-char seq, then the compiler will yell at you
            // See: https://en.cppreference.com/w/cpp/language/character_literal.html
            return {.kind = CharLiteral,
                    .str = take_remove_while(remaining_str, std::not_fn(is_strlike_end('\''), char_literal_start + 1))};
        }

        if (is_string_literal) {
            auto str_literal_start = remaining_str.find('"');
            ASSERT(str_literal_start != std::string_view::npos);

            // Check all characters before the first (required) '"'
            // If one of them is an 'R', then it's a raw string literal
            bool is_raw_str_literal =
                ranges::contains(remaining_str | views::take_while([](char c) { return c != '"'; }), "R");

            if (!is_raw_str_literal) {
                // take chars UNTIL (i.e., "while not") the end of the string
                // Starts after the first '"' to not get a false positive
                return {.kind = StringLiteral,
                        .str =
                            take_remove_while(remaining_str, std::not_fn(is_strlike_end('"')), str_literal_start + 1)};
            }

            // raw string literal

            // find first '('; this terminates the d_char_seq of the raw string literal
            auto raw_str_start = remaining_str.find('(');
            ASSERT(raw_str_start != std::string_view::npos);

            auto d_char_seq_len = raw_str_start - str_literal_start - 1;

            // content between '"' and '('
            std::string_view d_char_seq{remaining_str.begin() + str_literal_start + 1,
                                        remaining_str.begin() + raw_str_start};

            // take chars UNTIL (i.e., "while not") the end of the raw string,
            // which is delimited by `<d_char_seq>)"`
            return {.kind = RawStringLiteral,
                    .str = take_remove_while(remaining_str, std::not_fn(is_rstr_end(d_char_seq)))};
        }

        // At this point, we have one of:
        //  - Seperator
        //  - Operator
        //  - Identifier
        //  - IntLiteral
        //  - FloatingPointLiteral

        if (remaining_str.length() >= 2 && remaining_str.starts_with('.') && safe_isdigit(remaining_str.at(1))) {
            return {FloatingPointLiteral};
        }

        if (safe_isdigit(remaining_str.at(0))) {
            auto after_digits_iter = ranges::views::drop_while(safe_isdigit);

            // str ends after digits => IntLiteral
            if (after_digits_iter == ranges::end(remaining_str)) {
                return IntLiteral;
            }

            char last_lower = safe_tolower(*after_digits_iter);
            if (last_lower == 'e' || last_lower == '.') {
                return FloatingPointLiteral;
            }

            return IntLiteral;
        }
    };
}

} // namespace tokenize

} // namespace asmgrader::inspection
