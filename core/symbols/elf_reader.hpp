#pragma once

#include "symbols/symbol.hpp"
#include "symbols/symbol_table.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <elf.h>
#include <gelf.h>
#include <libelf.h>

/// Adaptor to (a few) libelf functions
class ElfReader
{
public:
    explicit ElfReader(const std::string& file_path);

    ~ElfReader();

    // Get the names of all sections
    std::vector<std::string> get_section_names() const;

    std::vector<Symbol> get_symbols() const;
    SymbolTable get_symbol_table() const;

    // FIXME: Temp
    std::vector<std::string> get_symbol_names() const;

private:
    struct SectionHandle
    {
        Elf_Scn* scn{};
        GElf_Shdr shdr{};
    };
    std::vector<SectionHandle> get_all_sections() const;

    static inline bool is_elf_init = false;

    int fd_{};
    Elf* elf_{};

    /// Index of the section header string table
    /// Used to get section name strings like ".data", ".symtab", ".text"
    std::size_t shdr_string_table_idx_{};
};
