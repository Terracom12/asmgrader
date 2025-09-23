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
constexpr auto charlit = std::bind_front(make_token, CharLiteral);
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
constexpr auto op2 = std::bind_front(make_token, BinaryOperator);

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
    STATIC_REQUIRE(Tokenizer("'a'") == toks(charlit("'a'")));
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
    STATIC_REQUIRE(Tokenizer("f<x, y>") == toks(id("f"), grp("<"), id("x"), op2(","), id("y"), grp(">")));

    // clang-format off
    SECTION("Parenthesis") {
        STATIC_REQUIRE(Tokenizer("(f(123, 456))") == toks(
            grp("("), 
                id("f"), op("("), int10lit("123"), op2(","), int10lit("456"), op(")"), 
            grp(")")
        ));

        STATIC_REQUIRE(Tokenizer("(f(123) + (3 * 2))") == toks(
            grp("("),
                id("f"), op("("), int10lit("123"), op(")"), 
                op("+"), 
                grp("("), 
                    int10lit("3"), op2("*"), int10lit("2"), 
                grp(")"), 
            grp(")")
        ));
    }

    SECTION("Angled brackets as tparam enclosers") {
        STATIC_REQUIRE(Tokenizer("f<x> < g<y>") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op2("<"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> > g<y>") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op2(">"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> > g") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op2(">"),
            id("g")
        ));
        STATIC_REQUIRE(Tokenizer("f<x> < g") == toks(
            id("f"), grp("<"), id("x"), grp(">"),
            op2("<"),
            id("g")
        ));
        STATIC_REQUIRE(Tokenizer("f > g<y>") == toks(
            id("f"),
            op2(">"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
        STATIC_REQUIRE(Tokenizer("f < g<y>") == toks(
            id("f"),
            op2("<"),
            id("g"), grp("<"), id("y"), grp(">")
        ));
    }


    SECTION("Proper parsing of angled brackets in bitwise shift op + less/greater comparison") {
        // test *reasonable* use-cases for bitwise shift and comparison combinations
        // that's going to be hell to properly parse if I try later, if it's even possible without proper context
        STATIC_REQUIRE(Tokenizer("x << 2 > y") == toks(
            id("x"), op2("<<"), int10lit("2"),
            op2(">"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x << 2 < y") == toks(
            id("x"), op2("<<"), int10lit("2"),
            op2("<"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x >> 2 > y") == toks(
            id("x"), op2(">>"), int10lit("2"),
            op2(">"),
            id("y")
        ));
        STATIC_REQUIRE(Tokenizer("x >> 2 < y") == toks(
            id("x"), op2(">>"), int10lit("2"),
            op2("<"),
            id("y")
        ));

        STATIC_REQUIRE(Tokenizer("y < x << 2") == toks(
            id("y"),
            op2("<"),
            id("x"), op2("<<"), int10lit("2")
        ));
        STATIC_REQUIRE(Tokenizer("y > x << 2") == toks(
            id("y"),
            op2(">"),
            id("x"), op2("<<"), int10lit("2")
        ));

        // reasonable solution to the ambiguity here:
        //   force the use of parenthesis around subexpressions containing << / >> when nested within a comparison
        // I can't think of a way to actually enforce this right now, so hopefully I remember to documment it well
        STATIC_REQUIRE(Tokenizer("y < (x >> 2)") == toks(
            id("y"),
            op2("<"),
            grp("("),
                id("x"), op2(">>"), int10lit("2"),
            grp(")")
        ));

        STATIC_REQUIRE(Tokenizer("y > x >> 2") == toks(
            id("y"),
            op2(">"),
            id("x"), op2(">>"), int10lit("2")
        ));
    }
    // clang-format on
}

TEST_CASE("Operator tokens") {
    STATIC_REQUIRE(Tokenizer("f(x)") == toks(id("f"), op("("), id("x"), op(")")));
    STATIC_REQUIRE(Tokenizer("f (x)") == toks(id("f"), op("("), id("x"), op(")")));

    STATIC_REQUIRE(Tokenizer("1<<2") == toks(int10lit("1"), op2("<<"), int10lit("2")));
    STATIC_REQUIRE(Tokenizer("1 >> 2") == toks(int10lit("1"), op2(">>"), int10lit("2")));

    STATIC_REQUIRE(Tokenizer("true ? a : b") == toks(boollit("true"), op("?"), id("a"), op(":"), id("b")));

    STATIC_REQUIRE(Tokenizer("a <=> b") == toks(id("a"), op2("<=>"), id("b")));

    STATIC_REQUIRE(Tokenizer("std::cout") == toks(id("std"), op2("::"), id("cout")));

    // clang-format off
    STATIC_REQUIRE(Tokenizer("a <= b && c >= d") == toks(
        id("a"), op2("<="), id("b"),
        op2("&&"),
        id("c"), op2(">="), id("d")
    ));
    STATIC_REQUIRE(Tokenizer("a < b || c >= d") == toks(
        id("a"), op2("<"), id("b"),
        op2("||"),
        id("c"), op2(">="), id("d")
    ));
    STATIC_REQUIRE(Tokenizer("a[10]->b.c.*d->*e") == toks(
        id("a"), op("["), int10lit("10"), op("]"),
        op2("->"), id("b"),
        op2("."), id("c"),
        op2(".*"), id("d"),
        op2("->*"), id("e")
    ));
    // clang-format on
}

TEST_CASE("Literal tokens") {
    STATIC_REQUIRE(Tokenizer("0b1010") == toks(int2lit("0b1010")));
    STATIC_REQUIRE(Tokenizer("0") == toks(int8lit("0")));
    STATIC_REQUIRE(Tokenizer("0755") == toks(int8lit("0755")));
    STATIC_REQUIRE(Tokenizer("1'999'543") == toks(int10lit("1'999'543")));
    STATIC_REQUIRE(Tokenizer("0x123ABCDEF321") == toks(int16lit("0x123ABCDEF321")));

    STATIC_REQUIRE(Tokenizer("1e5") == toks(floatlit("1e5")));
    STATIC_REQUIRE(Tokenizer("1.f") == toks(floatlit("1.f")));
    STATIC_REQUIRE(Tokenizer(".1e-2l") == toks(floatlit(".1e-2l")));
    STATIC_REQUIRE(Tokenizer("0x123.0xABCp10") == toks(float16lit("0x123.0xABCp10")));

    STATIC_REQUIRE(Tokenizer("'h'") == toks(charlit("'h'")));
    STATIC_REQUIRE(Tokenizer(R"(u8'\\')") == toks(charlit(R"(u8'\\')")));
    STATIC_REQUIRE(Tokenizer(R"(L'\'\\\'')") == toks(charlit(R"(L'\'\\\'')")));

    STATIC_REQUIRE(Tokenizer(R"("abc" "123\n\t")") == toks(strlit(R"("abc")"), strlit(R"("123\n\t")")));

    STATIC_REQUIRE(Tokenizer(R"-(R"(abc)" R"defg1(123\"\)defg1")-") ==
                   toks(rstrlit(R"-(R"(abc)")-"), rstrlit(R"-(R"defg1(123\"\)defg1")-")));

    STATIC_REQUIRE(Tokenizer(R"-("CT"_static R"(VIEW)"sv)-") ==
                   toks(strlit(R"("CT")"), id("_static"), rstrlit(R"-(R"(VIEW)")-"), id("sv")));
}

// test the kinds of cases users of this library might have
TEST_CASE("Basic use-case tokenization") {
    // clang-format off
    STATIC_REQUIRE(Tokenizer(R"(std::cout << "Hello, world!" << '\n')") == toks(
        id("std"), op2("::"), id("cout"),
        op2("<<"),
        strlit(R"("Hello, world!")"),
        op2("<<"),
        charlit(R"('\n')")
    ));

    STATIC_REQUIRE(Tokenizer("x < y") == toks(
        id("x"),
        op2("<"),
        id("y")
    ));

    STATIC_REQUIRE(Tokenizer("umask == 0755 || (umask & 07) == 1") == toks(
        id("umask"), op2("=="), int8lit("0755"),
        op2("||"),
        grp("("), 
            id("umask"), op2("&"), int8lit("07"),
        grp(")"),
        op2("=="),
        int10lit("1")
    ));

    STATIC_REQUIRE(Tokenizer("V == true") == toks(
        id("V"), op2("=="), boollit("true")
    ));

    STATIC_REQUIRE(Tokenizer("!V") == toks(
        op("!"), id("V")
    ));

    STATIC_REQUIRE(Tokenizer("X30 & 0x8000000000000000") == toks(
        id("X30"), op2("&"), int16lit("0x8000000000000000")
    ));

    STATIC_REQUIRE(Tokenizer("X30 == -1000") == toks(
        id("X30"), op2("=="), op("-"), int10lit("1000")
    ));

    STATIC_REQUIRE(Tokenizer("my_func(10)") == toks(
        id("my_func"), op("("), int10lit("10"), op(")")
    ));

    STATIC_REQUIRE(Tokenizer(R"(str_func("Thing?", 6))") == toks(
        id("str_func"), op("("), strlit(R"("Thing?")"), op2(","), int10lit("6"), op(")")
    ));

    STATIC_REQUIRE(Tokenizer(R"(str_func("Thing?", 2) == "Th"sv)") == toks(
        id("str_func"), op("("), strlit(R"("Thing?")"), op2(","), int10lit("2"), op(")"),
        op2("=="),
        strlit(R"("Th")"),
        id("sv")
    ));

    STATIC_REQUIRE(Tokenizer("syscalls.size() == test_cases.size()") == toks(
        id("syscalls"), op2("."), id("size"), op("("), op(")"),
        op2("=="),
        id("test_cases"), op2("."), id("size"), op("("), op(")")
    ));

    STATIC_REQUIRE(Tokenizer("res.get_code() == 0") == toks(
        id("res"), op2("."), id("get_code"), op("("), op(")"),
        op2("=="),
        int8lit("0")
    ));

    // potential ReGex matching operator/ in the future
    STATIC_REQUIRE(Tokenizer(R"---(str / R"(\w+\\_([0-9]*)?\.exe?)")---") == toks(
        id("str"),
        op2("/"),
        rstrlit(R"---(R"(\w+\\_([0-9]*)?\.exe?)")---")
    ));
    // clang-format on
}

TEST_CASE("Binary operator distinction") {
    // clang-format off

    STATIC_REQUIRE(Tokenizer("+123") == toks(
        op("+"), int10lit("123")
    ));

    STATIC_REQUIRE(Tokenizer("::std::string") == toks(
        op("::"), id("std"), op2("::"), id("string")
    ));

    STATIC_REQUIRE(Tokenizer("-123") == toks(
        op("-"), int10lit("123")
    ));

    STATIC_REQUIRE(Tokenizer("123 + 456") == toks(
        int10lit("123"), op2("+"), int10lit("456")
    ));

    STATIC_REQUIRE(Tokenizer("123 + -456") == toks(
        int10lit("123"), op2("+"), op("-"), int10lit("456")
    ));

    STATIC_REQUIRE(Tokenizer("123 + +456") == toks(
        int10lit("123"), op2("+"), op("+"), int10lit("456")
    ));

    STATIC_REQUIRE(Tokenizer("123 + ++456") == toks(
        int10lit("123"), op2("+"), op("++"), int10lit("456")
    ));

    STATIC_REQUIRE(Tokenizer("+123 < -456") == toks(
        op("+"), int10lit("123"), op2("<"), op("-"), int10lit("456")
    ));

    STATIC_REQUIRE(Tokenizer("true ? std::true_type{} : ::std::false_type{}") == toks(
        boollit("true"), 
        op("?"), 
        id("std"), op2("::"), id("true_type"), grp("{"), grp("}"), 
        op(":"),
        op("::"), id("std"), op2("::"), id("false_type"), grp("{"), grp("}")
    ));

    // clang-format on
}
