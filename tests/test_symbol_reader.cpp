#include "catch2_custom.hpp"

#include "symbols/elf_reader.hpp"
#include "symbols/symbol.hpp"
#include "symbols/symbol_table.hpp"

#include <range/v3/algorithm.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <iterator>

TEST_CASE("Find symbols in ASM_TESTS_EXEC") {
    // Should include `_start` and `strHello` where `strHello` is addressed AFTER `_start`
    const auto symbols = asmgrader::ElfReader(ASM_TESTS_EXEC).get_symbols();
    const auto symbol_table = asmgrader::SymbolTable(symbols);

    auto name_eq = [](auto name) { return [name](asmgrader::Symbol s) { return s.name == name; }; };

    REQUIRE(symbols.size() >= 2);

    REQUIRE(ranges::find_if(symbols, name_eq("_start")) != end(symbols));
    REQUIRE(ranges::find_if(symbols, name_eq("strHello")) != end(symbols));
    REQUIRE(ranges::find_if(symbols, name_eq("strGoodbye")) != end(symbols));

    REQUIRE(symbol_table.find("_start").has_value());
    REQUIRE(symbol_table.find("strHello").has_value());

    auto strHello_addr = symbol_table.find("strHello")->address;

    REQUIRE(symbol_table.find_closest_above(strHello_addr).value_or(asmgrader::Symbol{}).name == "_start");
}
