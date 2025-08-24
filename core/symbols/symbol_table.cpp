#include "symbols/symbol_table.hpp"

#include "logging.hpp"

#include <gsl/assert>
#include <range/v3/algorithm.hpp>
#include <range/v3/iterator/insert_iterators.hpp>

#include <cstdint>

namespace asmgrader {

SymbolTable::SymbolTable(const std::vector<Symbol>& symbols) {
    // Only worry about static symbols for now, and discard any unnamed symbols
    ranges::copy_if(symbols, ranges::back_inserter(symbols_),
                    [](const Symbol& sym) { return sym.kind == Symbol::Static && !sym.name.empty(); });

    Ensures(symbols_.size() > 0);
}

std::optional<Symbol> SymbolTable::find(std::string_view name) const {
    for (const auto& sym : symbols_) {
        if (sym.name == name) {
            return sym;
        }
    }

    return std::nullopt;
}

std::optional<Symbol> SymbolTable::find_closest_above(std::uintptr_t address) const {
    std::optional<Symbol> result;
    std::uintptr_t best_addr = 0x0; // closest addr we've seen so far
                                    // guaranteed to ALWAYS be >= address

    for (const auto& sym : symbols_) {
        // Symbol address is better than best so far, and meets constraints of being at or below address
        if (sym.address > best_addr && sym.address <= address) {
            result = sym;
        }
    }

    return result;
}

} // namespace asmgrader
