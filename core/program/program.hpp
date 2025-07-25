#pragma once

#include "subprocess/run_result.hpp"
#include "subprocess/traced_subprocess.hpp"
#include "subprocess/tracer.hpp"
#include "symbols/symbol_table.hpp"
#include "util/class_traits.hpp"
#include "util/error_types.hpp"

#include <csignal>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
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
    SymbolTable& get_symtab();

    /// Returns the result of the function call, or nullopt if the symbol name was not found
    /// or some other error occured
    template <typename Func, typename... Args>
    util::Result<std::invoke_result_t<Func, Args...>> call_function(std::string_view name, const Args&... args);

private:
    void check_is_elf() const;

    std::filesystem::path path_;
    std::vector<std::string> args_;

    std::unique_ptr<TracedSubprocess> subproc_;
    std::unique_ptr<SymbolTable> symtab_;
};

template <typename Func, typename... Args>
util::Result<std::invoke_result_t<Func, Args...>> Program::call_function(std::string_view name, const Args&... args) {
    using Ret = std::invoke_result_t<Func, Args...>;

    auto symbol = symtab_->find(name);
    if (!symbol) {
        return util::ErrorKind::UnresolvedSymbol;
    }

    Tracer& tracer = subproc_->get_tracer();
    tracer.setup_function_call(args...);

#ifdef __aarch64__
    std::uintptr_t instr_pointer = tracer.get_registers().pc;
#else // x86_64 assumed
    std::uintptr_t instr_pointer = tracer.get_registers().rip;
#endif
    LOG_TRACE("Jumping to: {}@{:#X} from {:#X}", symbol->name, symbol->address, instr_pointer);
    tracer.jump_to(symbol->address);

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
        return util::ErrorKind::UnexpectedReturn;
    }

    if constexpr (std::same_as<Ret, void>) {
        return {};
    } else {
        std::optional<Ret> return_val = tracer.process_function_ret<Ret>();

        if (!return_val) {
            return util::ErrorKind::UnknownError;
        }

        return *return_val;
    }
}
