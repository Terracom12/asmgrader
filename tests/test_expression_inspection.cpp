#include "catch2_custom.hpp"

#include "api/expression_inspection.hpp"

#include <catch2/catch_test_macros.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <concepts>
#include <functional>
#include <string_view>

using asmgrader::inspection::Token, asmgrader::inspection::Tokenizer;

using enum Token::Kind;

// Helper functions for a shorthand way of creating tokens
constexpr Token make_token(Token::Kind kind, std::string_view str) {
    return {.kind = kind, .str = str};
}

constexpr auto strlit = std::bind_front(make_token, StringLiteral);
constexpr auto rstrlit = std::bind_front(make_token, RawStringLiteral);
constexpr auto chrlit = std::bind_front(make_token, CharLiteral);
constexpr auto boollit = std::bind_front(make_token, BoolLiteral);
constexpr auto int2lit = bind_front(make_token, IntBinLiteral);
constexpr auto int8lit = bind_front(make_token, IntOctLiteral);
constexpr auto int10lit = bind_front(make_token, IntDecLiteral);
constexpr auto int16lit = bind_front(make_token, IntHexLiteral);
constexpr auto floatlit = bind_front(make_token, FloatLiteral);
constexpr auto float16lit = bind_front(make_token, FloatHexLiteral);
constexpr auto id = std::bind_front(make_token, Identifier);
constexpr auto grp = std::bind_front(make_token, Grouping);
constexpr auto op = std::bind_front(make_token, Operator);

template <std::same_as<Token>... Tokens>
constexpr auto toks(const Tokens&... tokens) {
    if constexpr (sizeof...(Tokens) == 0) {
        return std::array<Token, 0>{};
    } else {
        return std::array{tokens...};
    }
}

// Transform a range of tokens into a view of token kinds
constexpr auto kinds(const ranges::forward_range auto& toks) {
    return toks | ranges::views::transform([](const Token& tok) { return tok.kind; });
}

TEST_CASE("Single token strings") {
    STATIC_REQUIRE(Tokenizer("") == toks());

    STATIC_REQUIRE(Tokenizer(R"("hi")") == toks(strlit(R"("hi")")));
    STATIC_REQUIRE(Tokenizer(R"-(R"(\hi\)")-") == toks(rstrlit(R"-(R"(\hi\)")-")));
    STATIC_REQUIRE(Tokenizer("'a'") == toks(chrlit("'a'")));
    STATIC_REQUIRE(Tokenizer("true") == toks(boollit("true")));
    STATIC_REQUIRE(Tokenizer("123") == toks(int10lit("123")));
    STATIC_REQUIRE(Tokenizer("123.456e2") == toks(floatlit("123.456e2")));

    STATIC_REQUIRE(Tokenizer("a") == toks(id("a")));
    STATIC_REQUIRE(Tokenizer("a123_CD") == toks(id("a123_CD")));
    STATIC_REQUIRE(Tokenizer("{") == toks(grp("{")));
    STATIC_REQUIRE(Tokenizer("++") == toks(op("++")));
    STATIC_REQUIRE(Tokenizer("static_cast") == toks(op("static_cast")));
}

TEST_CASE("Grouping tokens") {
    STATIC_REQUIRE(Tokenizer("{ }") == toks(grp("{"), grp("}")));

    STATIC_REQUIRE(Tokenizer("(x)") == toks(grp("("), id("x"), grp(")")));

    STATIC_REQUIRE(Tokenizer("((x))") == toks(grp("("), grp("("), id("x"), grp(")"), grp(")")));
    STATIC_REQUIRE(Tokenizer("(f(123))") == toks(grp("("), id("f"), op("("), int10lit("123"), op(")"), grp(")")));

    STATIC_REQUIRE(Tokenizer("f<x>") == toks(id("f"), grp("<"), id("x"), grp(">")));
    STATIC_REQUIRE(Tokenizer("f<x, y>") == toks(id("f"), grp("<"), id("x"), op(","), id("y"), grp(">")));

    // clang-format off
    SECTION("Parenthesis") {
        STATIC_REQUIRE(Tokenizer("(f(123, 456))") == toks(
            grp("("), 
                id("f"), op("("), int10lit("123"), op(","), int10lit("456"), op(")"), 
            grp(")")
        ));

        STATIC_REQUIRE(Tokenizer("(f(123) + (3 * 2))") == toks(
            grp("("),
                id("f"), op("("), int10lit("123"), op(")"), 
                op("+"), 
                grp("("), 
                    int10lit("3"), op("*"), int10lit("2"), 
                grp(")"), 
            grp(")")
        ));
    }

    SECTION("Angled brackets as tparam enclosers") {
        STATIC_REQUIRE(Tokenizer("f<x> < g<y>") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op("<"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> > g<y>") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op(">"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> > g") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op(">"),
            id("g")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> < g") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op("<"),
            id("g")
        ));
        STATIC_REQUIRE(Tokenizer("f > g<y>") == toks(
            id("f"),
            op(">"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f < g<y>") == toks(
            id("f"),
            op("<"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
    }


    SECTION("Proper parsing of angled brackets in bitwise shift op + less/greater comparison") {
        // test *reasonable* use-cases for bitwise shift and comparison combinations
        // that's going to be hell to properly parse if I try later, if it's even possible without proper context
        STATIC_REQUIRE(Tokenizer("x << 2 > y") == toks(
            id("x"), op("<<"), int10lit("2"),
            op(">"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x << 2 < y") == toks(
            id("x"), op("<<"), int10lit("2"),
            op("<"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x >> 2 > y") == toks(
            id("x"), op(">>"), int10lit("2"),
            op(">"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x >> 2 < y") == toks(
            id("x"), op(">>"), int10lit("2"),
            op("<"),
            id("y")
        ));

        STATIC_REQUIRE(Tokenizer("y < x << 2") == toks(
            id("y"),
            op("<"),
            id("x"), op("<<"), int10lit("2")
        ));
        STATIC_REQUIRE(Tokenizer("y > x << 2") == toks(
            id("y"),
            op(">"),
            id("x"), op("<<"), int10lit("2")
        ));

        // reasonable solution to the ambiguity here:
        //   force the use of parenthesis around subexpressions containing << / >> when nested within a comparison
        // I can't think of a way to actually enforce this right now, so hopefully I remember to documment it well
        STATIC_REQUIRE(Tokenizer("y < (x >> 2)") == toks(
            id("y"),
            op("<"),
            grp("("),
                id("x"), op(">>"), int10lit("2"),
            grp(")")
        ));

        STATIC_REQUIRE(Tokenizer("y > x >> 2") == toks(
            id("y"),
            op(">"),
            id("x"), op(">>"), int10lit("2")
        ));
    }
    // clang-format on
}

TEST_CASE("Operator tokens") {
    STATIC_REQUIRE(Tokenizer("f(x)") == toks(id("f"), op("("), id("x"), op(")")));
    STATIC_REQUIRE(Tokenizer("f (x)") == toks(id("f"), op("("), id("x"), op(")")));

    STATIC_REQUIRE(Tokenizer("1<<2") == toks(int10lit("1"), op("<<"), int10lit("2")));
    STATIC_REQUIRE(Tokenizer("1 >> 2") == toks(int10lit("1"), op(">>"), int10lit("2")));

    STATIC_REQUIRE(Tokenizer("true ? a : b") == toks(boollit("true"), op("?"), id("a"), op(":"), id("b")));

    STATIC_REQUIRE(Tokenizer("a <=> b") == toks(id("a"), op("<=>"), id("b")));

    STATIC_REQUIRE(Tokenizer("std::cout") == toks(id("std"), op("::"), id("cout")));

    // clang-format off
    STATIC_REQUIRE(Tokenizer("a <= b && c >= d") == toks(
        id("a"), op("<="), id("b"),
        op("&&"),
        id("c"), op(">="), id("d")
    ));
    STATIC_REQUIRE(Tokenizer("a < b || c >= d") == toks(
        id("a"), op("<"), id("b"),
        op("||"),
        id("c"), op(">="), id("d")
    ));
    STATIC_REQUIRE(Tokenizer("a[10]->b.c.*d->*e") == toks(
        id("a"), op("["), int10lit("10"), op("]"),
        op("->"), id("b"),
        op("."), id("c"),
        op(".*"), id("d"),
        op("->*"), id("e")
    ));
    // clang-format on
}

TEST_CASE("Literal tokens") {
    STATIC_REQUIRE(Tokenizer(R"("abc" "123\n\t")") == toks(strlit(R"("abc")"), strlit(R"("123\n\t")")));

    STATIC_REQUIRE(Tokenizer(R"-(R"(abc)" R"defg1(123\"\)defg1")-") ==
                   toks(rstrlit(R"-(R"(abc)")-"), rstrlit(R"-(R"defg1(123\"\)defg1")-")));

    STATIC_REQUIRE(Tokenizer(R"-("CT"_static R"(VIEW)"sv)-") ==
                   toks(strlit(R"("CT")"), id("_static"), rstrlit(R"-(R"(VIEW)")-"), id("sv")));
}
