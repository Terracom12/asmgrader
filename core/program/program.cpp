#include "program/program.hpp"

#include "subprocess/traced_subprocess.hpp"
#include "symbols/elf_reader.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

Program::Program(std::filesystem::path path, std::vector<std::string> args)
    : path_{std::move(path)}
    , args_{std::move(args)} {

    if (std::error_code err; !is_regular_file(path_, err)) {
        LOG_ERROR("File {:?} does not exist for Program ({})", path_.string(), err);
        throw std::runtime_error("Program file does not exist");
    }

    check_is_elf();

    auto elf_reader = ElfReader(path_.string());
    symtab_ = std::make_unique<SymbolTable>(elf_reader.get_symbol_table());

    subproc_ = std::make_unique<TracedSubprocess>(path_.string(), args);
    std::ignore = subproc_->start();
}

void Program::check_is_elf() const {
    std::ifstream file(path_);
    std::array<char, 4> first_4_bytes{};
    file.read(first_4_bytes.data(), first_4_bytes.size());

    constexpr std::array<char, 4> EXPECTED_BYTES = {0x7F, 'E', 'L', 'F'};
    if (first_4_bytes != EXPECTED_BYTES) {
        LOG_ERROR("File {:?} does not match ELF spec (first 4 bytes are {:?})", path_.string(),
                  std::string{first_4_bytes.begin(), first_4_bytes.end()});
        throw std::runtime_error("Program file is not an ELF");
    }
}

TracedSubprocess& Program::get_subproc() {
    return *subproc_;
}

SymbolTable& Program::get_symtab() {
    return *symtab_;
}
util::Result<RunResult> Program::run() {
    return subproc_->run();
}
