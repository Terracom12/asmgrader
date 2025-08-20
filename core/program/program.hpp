#pragma once

#include "meta/functional_traits.hpp"
#include "subprocess/memory/concepts.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/traced_subprocess.hpp"
#include "subprocess/tracer.hpp"
#include "symbols/symbol_table.hpp"
#include "common/class_traits.hpp"
#include "common/error_types.hpp"
#include "common/expected.hpp"

#include <fmt/format.h>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Program : util::NonCopyable
{
public:
    explicit Program(std::filesystem::path path, std::vector<std::string> args = {});

    Program(Program&& other) = default;
    Program& operator=(Program&& rhs) = default;

    ~Program() = default;

    util::Result<RunResult> run();

    TracedSubprocess& get_subproc();
    const TracedSubprocess& get_subproc() const;
    SymbolTable& get_symtab();
    const SymbolTable& get_symtab() const;

    const std::filesystem::path& get_path() const;
    const std::vector<std::string>& get_args() const;

    /// Returns the result of the function call, or nullopt if the symbol name was not found
    /// or some other error occured
    template <typename Func, typename... Args>
    util::Result<typename util::FunctionTraits<Func>::Ret> call_function(std::string_view name, Args&&... args);

    template <typename Func, typename... Args>
    util::Result<typename util::FunctionTraits<Func>::Ret> call_function(std::uintptr_t addr, Args&&... args);

    // TODO: Proper allocation (and deallocation!!)
    std::uintptr_t alloc_mem(std::size_t amt);

    static util::Expected<void, std::string> check_is_elf(const std::filesystem::path& path);

private:
    std::filesystem::path path_;
    std::vector<std::string> args_;

    std::unique_ptr<TracedSubprocess> subproc_;
    std::unique_ptr<SymbolTable> symtab_;

    std::size_t alloced_mem_{};
};

template <typename Func, typename... Args>
util::Result<typename util::FunctionTraits<Func>::Ret> Program::call_function(std::uintptr_t addr, Args&&... args) {
    using Ret = typename util::FunctionTraits<Func>::Ret;

    Tracer& tracer = subproc_->get_tracer();
    TRY(tracer.setup_function_call(std::forward<Args>(args)...));

    if (auto res = subproc_->get_tracer().get_memory_io().read_bytes(subproc_->get_tracer().get_mmapped_addr(), 32);
        res) {
        LOG_TRACE("Memory (32 bytes) at start of mmaped addr (0x{:x}): {::x}",
                  subproc_->get_tracer().get_mmapped_addr(), *res);
    }

#ifdef __aarch64__
    std::uintptr_t instr_pointer = TRY(tracer.get_registers()).pc;
#else // x86_64 assumed
    std::uintptr_t instr_pointer = TRY(tracer.get_registers()).rip;
#endif
    LOG_TRACE("Jumping to: {:#X} from {:#X}", addr, instr_pointer);
    TRY(tracer.jump_to(addr));

    auto run_res = tracer.run();

    if (!run_res) {
        return run_res.error();
    }

    using enum RunResult::Kind;

    // If the subprocess is no longer alive, restart it
    // TODO: Should instead catch exit condition before it has an effect
    if (run_res->get_kind() == Exited || run_res->get_kind() == Killed) {
        TRY(subproc_->restart());
    }

    // We expect that the function returns to our written instruction, which is essentially a breakpoint
    // that raises SIGTRAP
    if (run_res->get_kind() != SignalCaught || run_res->get_code() != SIGTRAP) {
        LOG_DEBUG("Unexpected return from function: kind={}, code={}", fmt::underlying(run_res->get_kind()),
                  run_res->get_code());

        std::uintptr_t instr_addr =
#ifdef __aarch64__
            subproc_->get_tracer().get_registers()->pc;
#else
            subproc_->get_tracer().get_registers()->rip;
#endif
        if (auto res = subproc_->get_tracer().get_memory_io().read_bytes(instr_addr, 16); res) {
            LOG_TRACE("Memory (16 bytes) at point of instruction ptr (0x{:x}): {::x}", instr_addr, *res);
        }

        return util::ErrorKind::UnexpectedReturn;
    }

    if constexpr (std::same_as<Ret, void>) {
        return {};
    } else {
        std::optional<Ret> return_val = TRY(tracer.process_function_ret<Ret>());

        if (!return_val) {
            return util::ErrorKind::UnknownError;
        }

        return *return_val;
    }
}

template <typename Func, typename... Args>
util::Result<typename util::FunctionTraits<Func>::Ret> Program::call_function(std::string_view name, Args&&... args) {
    auto symbol = symtab_->find(name);
    if (!symbol) {
        return util::ErrorKind::UnresolvedSymbol;
    }

    LOG_TRACE("Resolved symbol {:?} at {:#X}", symbol->name, symbol->address);

    return call_function<Func>(symbol->address, std::forward<Args>(args)...);
}
