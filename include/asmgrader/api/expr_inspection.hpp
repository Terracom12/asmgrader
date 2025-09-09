/// \file Implentation of inspection facilities for simple C++ expressions
/// Intended for usage with \ref `REQUIRE`
///
/// The ideas are heavily inspired by the libassert library, but the implementation is fully my own.
/// Why do this when an implementation already exists? Mostly cause parsing is kind of fun, and for a bit of learning.
#pragma once

#include <asmgrader/common/constexpr_cctype.hpp>

#include <libassert/assert.hpp>
#include <range/v3/algorithm/find_if_not.hpp>
#include <range/v3/range/concepts.hpp>

#include <concepts>
#include <cstddef>
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
        /// Not all defined, but should be rather obvious anyways.
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
