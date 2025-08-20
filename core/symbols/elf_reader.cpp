#include "symbols/elf_reader.hpp"

#include "logging.hpp"
#include "symbols/symbol.hpp"
#include "symbols/symbol_table.hpp"

#include <elfio/elf_types.hpp>
#include <elfio/elfio_section.hpp>
#include <elfio/elfio_symbols.hpp>
#include <fmt/format.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include <elf.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>

// If condition is true, print out the elf error and exit
#define ELF_ERROR(cond, msg) ASSERT(!(cond), "libelf error: " #msg " ({})", elf_errmsg(-1))

ElfReader::ElfReader(const std::string& file_path) {
    if (!elffile_.load(file_path)) {
        throw std::invalid_argument(fmt::format("Failed to load Elf file {}", file_path));
    }
}

std::vector<std::string> ElfReader::get_section_names() const {
    return elffile_.sections                                     //
           | ranges::views::transform(&ELFIO::section::get_name) //
           | ranges::to<std::vector>();
}

std::vector<Symbol> ElfReader::get_symbols() const {
    auto to_symbol_struct = [](Symbol& result, const ELFIO::symbol_section_accessor& accessor, unsigned int idx) {
        std::string name;
        ELFIO::Elf64_Addr value = 0;
        ELFIO::Elf_Xword size = 0;
        unsigned char bind = 0;
        unsigned char type = 0;
        ELFIO::Elf_Half section_index = 0;
        unsigned char other = 0;

        // Read symbol properties
        accessor.get_symbol(idx, name, value, size, bind, type, section_index, other);

        result.name = name;

        result.address = value;

        switch (bind) {
        case STB_LOCAL:
            result.binding = Symbol::Local;
            break;
        case STB_GLOBAL:
            result.binding = Symbol::Global;
            break;
        case STB_WEAK:
            result.binding = Symbol::Weak;
            break;
        default:
            result.binding = Symbol::Other;
        }

        return result;
    };

    std::vector<Symbol> result;

    // Keep track of whether we encountered any symbol sections for logging purposes
    bool found_sym_sect = false;

    // Iterate through every dynsym and symtab section
    for (const auto& sect : elffile_.sections) {
        // Adapted from: ELFIO/examples/tutorial

        Symbol symbol{};
        if (sect->get_type() == SHT_SYMTAB) {
            symbol.kind = Symbol::Static;
        } else if (sect->get_type() == SHT_DYNSYM) {
            symbol.kind = Symbol::Dynamic;
        } else {
            continue;
        }
        found_sym_sect = true;

        const ELFIO::symbol_section_accessor symbols(elffile_, sect.get());

        for (unsigned int i = 0; i < symbols.get_symbols_num(); i++) {
            to_symbol_struct(symbol, symbols, i);

            result.push_back(symbol);
        }
    }

    // No symtab section!
    if (!found_sym_sect) {
        LOG_DEBUG("No symtab or dynsym sections in elf file");
        return {};
    }

    return result;
}

SymbolTable ElfReader::get_symbol_table() const {
    return SymbolTable{get_symbols()};
}
