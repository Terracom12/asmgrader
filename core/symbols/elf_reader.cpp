#include "symbols/elf_reader.hpp"

#include "logging.hpp"
#include "symbols/symbol_table.hpp"
#include "common/linux.hpp"

#include <range/v3/algorithm.hpp>

#include <elf.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>

// TODO: Replace usages of size_t with uintptr_t

// If condition is true, print out the elf error and exit
#define ELF_ERROR(cond, msg) ASSERT(!(cond), "libelf error: " #msg " ({})", elf_errmsg(-1))

ElfReader::ElfReader(const std::string& file_path) {
    if (!is_elf_init) {
        ELF_ERROR(elf_version(EV_CURRENT) == EV_NONE, "Failed to init version");
    }
    is_elf_init = true;

    if (auto res = util::linux::open(file_path, O_RDONLY)) {
        fd_ = res.value();
    } else {
        LOG_ERROR("Elf file {} does not exist", file_path);
    }

    elf_ = elf_begin(fd_, ELF_C_READ, nullptr);

    ELF_ERROR(elf_ == nullptr, "Failed to init elf descriptor");

    ELF_ERROR(elf_getshdrstrndx(elf_, &shdr_string_table_idx_) != 0, "elf_getshdrstrndx failed");
}

ElfReader::~ElfReader() {
    elf_end(elf_);
}

// FIXME: Should probably only need to call this once, and just cache or smth
std::vector<std::string> ElfReader::get_section_names() const {
    auto sections = get_all_sections();
    std::vector<std::string> result(sections.size());

    ranges::transform(sections, result.begin(), [this](SectionHandle sect) -> std::string {
        const char* name = elf_strptr(elf_, shdr_string_table_idx_, sect.shdr.sh_name);
        return name;
    });

    return result;
}

std::vector<Symbol> ElfReader::get_symbols() const {
    auto load_sym = [this](const GElf_Shdr& shdr, const GElf_Sym& sym, Symbol& result) {
        const char* name = elf_strptr(elf_, shdr.sh_link, sym.st_name);

        if (name == nullptr) {
            LOG_DEBUG("NULL symbol name!");
        }

        result.name = name;

        result.address = sym.st_value;

        switch (GELF_ST_BIND(sym.st_info)) {
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

    auto sections = get_all_sections();

    // Keep track of whether we encountered any symbol sections for logging purposes
    bool found_sym_sect = false;

    // Iterate through every dynsym and symtab section
    for (const auto& sect : sections) {
        if (sect.shdr.sh_type != SHT_SYMTAB && sect.shdr.sh_type != SHT_DYNSYM) {
            continue;
        }

        found_sym_sect = true;

        Elf_Data* data = elf_getdata(sect.scn, nullptr);
        ELF_ERROR(data == nullptr, "elf_getdata failed");

        const auto num_entries = sect.shdr.sh_size / sect.shdr.sh_entsize;

        Symbol symbol{};
        if (sect.shdr.sh_type == SHT_SYMTAB) {
            symbol.kind = Symbol::Static;
        } else if (sect.shdr.sh_type == SHT_DYNSYM) {
            symbol.kind = Symbol::Dynamic;
        }

        for (std::size_t i = 0; i < num_entries; i++) {
            GElf_Sym sym{};
            ELF_ERROR(gelf_getsym(data, static_cast<int>(i), &sym) == nullptr, "gelf_getsym failed");

            load_sym(sect.shdr, sym, symbol);

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

std::vector<ElfReader::SectionHandle> ElfReader::get_all_sections() const {
    std::vector<ElfReader::SectionHandle> result;

    // Iterate through all sections with `scn`
    for (Elf_Scn* scn = elf_nextscn(elf_, nullptr); scn != nullptr; scn = elf_nextscn(elf_, scn)) {
        GElf_Shdr shdr{};
        ELF_ERROR(gelf_getshdr(scn, &shdr) == nullptr, "gelf_getshdr failed");

        result.push_back({.scn = scn, .shdr = shdr});
    }

    return result;
}
