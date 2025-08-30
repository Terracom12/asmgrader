#include "program/program.hpp"

#include "common/error_types.hpp"
#include "common/expected.hpp"
#include "logging.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/traced_subprocess.hpp"
#include "subprocess/tracer.hpp"
#include "symbols/elf_reader.hpp"
#include "symbols/symbol_table.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <libassert/assert.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace asmgrader {

Program::Program(std::filesystem::path path, std::vector<std::string> args)
    : path_{std::move(path)}
    , args_{std::move(args)} {

    if (std::error_code err; !is_regular_file(path_, err)) {
        LOG_ERROR("File {:?} does not exist for Program ({})", path_.string(), err);
        throw std::runtime_error("Program file does not exist");
    }

    if (auto is_elf = check_is_elf(path_); not is_elf) {
        throw std::runtime_error(is_elf.error());
    }

    auto elf_reader = ElfReader(path_.string());
    symtab_ = std::make_unique<SymbolTable>(elf_reader.get_symbol_table());

    subproc_ = std::make_unique<TracedSubprocess>(path_.string(), args);
    std::ignore = subproc_->start();
}

Expected<void, std::string> Program::check_is_elf(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::array<char, 4> first_4_bytes{};
    file.read(first_4_bytes.data(), first_4_bytes.size());

    constexpr std::array<char, 4> expected_bytes = {0x7F, 'E', 'L', 'F'};
    if (first_4_bytes == expected_bytes) {
        return {};
    }

    return fmt::format("File {:?} does not match ELF spec (first 4 bytes are {:?})", path.string(),
                       std::string_view{first_4_bytes.begin(), first_4_bytes.end()});
}

TracedSubprocess& Program::get_subproc() {
    return *subproc_;
}

const TracedSubprocess& Program::get_subproc() const {
    return *subproc_;
}

SymbolTable& Program::get_symtab() {
    return *symtab_;
}

const SymbolTable& Program::get_symtab() const {
    return *symtab_;
}

Result<RunResult> Program::run() {
    return subproc_->run();
}

std::uintptr_t Program::alloc_mem(std::size_t amt) {
    // Extra offset to not accidentally run into memory issues on border
    constexpr auto offset_addition = 32;

    auto offset = (alloced_mem_ += amt) + offset_addition;

    ASSERT(alloced_mem_ < Tracer::MMAP_LENGTH * 3 / 4, "Attempting to allocate too much memory in asm program!");

    // Simple stack-esque allocation:
    // Begin at final mmapped address then grow downwards
    return subproc_->get_tracer().get_mmapped_addr() + Tracer::MMAP_LENGTH - offset;
}

const std::filesystem::path& Program::get_path() const {
    return path_;
}

const std::vector<std::string>& Program::get_args() const {
    return args_;
}

} // namespace asmgrader
