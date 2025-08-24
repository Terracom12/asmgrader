#pragma once

#include "symbols/symbol.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace asmgrader {

/// A basic symbol table, for simple interaction with a number of symbols loaded from an ELF file
class SymbolTable
{
public:
    explicit SymbolTable(const std::vector<Symbol>& symbols);

    std::optional<Symbol> find(std::string_view name) const;

    /// Find the closest symbol at or below `address`
    /// If no symbol is at or above address, returns nullopt
    ///
    /// Useful for diagnostics such as:
    ///   "Program segfaulted at location putstring+0x42"
    std::optional<Symbol> find_closest_above(std::uintptr_t address) const;

private:
    std::vector<Symbol> symbols_;
};

} // namespace asmgrader
