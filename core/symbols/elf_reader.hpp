#pragma once

#include "symbols/symbol.hpp"
#include "symbols/symbol_table.hpp"

#include <elfio/elfio.hpp>

#include <string>
#include <vector>

#include <gelf.h>
#include <libelf.h>

namespace asmgrader {

/// Adaptor to (a few) libelf functions
class ElfReader
{
public:
    explicit ElfReader(const std::string& file_path);

    // Get the names of all sections
    std::vector<std::string> get_section_names() const;

    std::vector<Symbol> get_symbols() const;
    SymbolTable get_symbol_table() const;

    std::vector<std::string> get_symbol_names() const;

private:
    ELFIO::elfio elffile_;
};

} // namespace asmgrader
