#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/unreachable.hpp>
#include <asmgrader/meta/count_if.hpp>
#include <asmgrader/meta/tuple_matcher.hpp>
#include <asmgrader/subprocess/memory/concepts.hpp>
#include <asmgrader/subprocess/memory/memory_io.hpp>
#include <asmgrader/subprocess/run_result.hpp>
#include <asmgrader/subprocess/syscall.hpp>
#include <asmgrader/subprocess/syscall_record.hpp>
#include <asmgrader/subprocess/tracer_types.hpp>

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <linux/ptrace.h>
#include <sys/ptrace.h>
#include <sys/types.h> // pid_t
#include <sys/user.h>  // user_regs_struct, user_fpregs_struct (user_fpsimd_struct)

namespace asmgrader {

// HACK: Temporary fix for aarch64
#ifdef __aarch64__
using user_fpregs_struct = user_fpsimd_struct;
#endif

/// A lightweight wrapper of ptrace(2)
///
/// Instantiate and use this class in the parent process to trace a child.
/// In the child process, call Tracer::init_child.
class Tracer
{
public:
    constexpr static std::size_t MMAP_LENGTH = 4096;

    Tracer() = default;

    /// Sets up tracing in parent process, then stops child immediately after exec call
    Result<void> begin(pid_t pid);

    /// Run the child process. Collect syscall info each time one is executed
    Result<RunResult> run();

    /// Executes a syscall with the given arguments as the stopped tracee
    Result<SyscallRecord> execute_syscall(u64 sys_nr, std::array<std::uint64_t, 6> args);

    /// Get the general purpose registers of the stopped tracee
    /// IMPORTANT: this is (obviously) architecture-dependent
    Result<user_regs_struct> get_registers() const;
    Result<user_fpregs_struct> get_fp_registers() const;

    /// Set the general purpose registers of the stopped tracee
    /// IMPORTANT: this is (obviously) architecture-dependent
    Result<void> set_registers(user_regs_struct regs) const;
    Result<void> set_fp_registers(user_fpregs_struct regs) const;

    /// Obtain records of syscalls run so far in the child process
    const std::vector<SyscallRecord>& get_records() const { return syscall_records_; }

    /// Obtain the process exit code, or nullopt if the process has not yet exited
    std::optional<int> get_exit_code() const { return exit_code_; }

    /// Set up child process for tracing
    /// Call this within the newly-forked process
    ///
    /// Immediately after a call to this function should be a call to execve.
    static Result<void> init_child();

    /// Set the child process's instruction pointer to `address`
    Result<void> jump_to(std::uintptr_t address);

    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds{10};

    template <typename... Args>
    Result<void> setup_function_call(Args&&... args);

    /// AFTER a function has been called, inspects register values
    /// (and memory if necessary) to construct the expected return type.
    ///
    /// std::nullopt is returned if it's not possible to construct return type.
    template <typename Ret>
    Result<Ret> process_function_ret();

    MemoryIOBase& get_memory_io();

    std::uintptr_t get_mmapped_addr() const { return mmaped_address_; }

private:
    /// Ensure that invariants hold
    ///   pid_ parent is this process
    ///   pid_ state is not dead/zombie
    ///
    /// Failing this is a contract violation. Thus, the program crashes in the case
    /// that they do not hold
    void assert_invariants() const;

    Result<void> setup_function_return();

    /// Returns: value that should be written to the nth register
    template <typename Arg>
    Result<u64> setup_function_param(const Arg& arg);
    template <std::floating_point Arg>
    auto setup_function_param(const Arg& arg);

    /**
     * @brief Resumes the child proc until a condition is encountered, or a timeout occurs
     *
     * Blocking for up to the duration of ``timeout``.
     *
     * @tparam Func - type of ``pred``
     * @param pred
     * @param request
     */
    Result<TracedWaitid> resume_until(const std::function<bool(TracedWaitid)>& pred,
                                      std::chrono::microseconds timeout = DEFAULT_TIMEOUT,
                                      int ptrace_request = PTRACE_CONT) const;

    Result<SyscallRecord> run_next_syscall(std::chrono::microseconds timeout = DEFAULT_TIMEOUT) const;

    /// Precondition: child process must be stopped after waitid(2) returned a syscall trap event
    SyscallRecord get_syscall_entry_info(struct ptrace_syscall_info* entry) const;
    void get_syscall_exit_info(SyscallRecord& rec, struct ptrace_syscall_info* exit) const;

    // template <typename T>
    // T from_raw_value(u64 value) const;

    SyscallRecord::SyscallArg from_syscall_value(u64 value, SyscallEntry::Type type) const;

    pid_t pid_ = -1;

    std::unique_ptr<MemoryIOBase> memory_io_;

    std::vector<SyscallRecord> syscall_records_;

    std::optional<int> exit_code_;

    std::size_t mmaped_address_{};

    std::size_t mmaped_used_amt_{};
};

template <typename... Args>
Result<void> Tracer::setup_function_call(Args&&... args) {
    constexpr std::size_t NUM_FP_ARGS = count_if_v<std::is_floating_point, Args...>;
    constexpr std::size_t NUM_INT_ARGS = sizeof...(Args) - NUM_FP_ARGS;

#ifdef __aarch64__
    static_assert(NUM_FP_ARGS <= 8 && NUM_INT_ARGS <= 6, "Stack parameters are not yet supported for aarch64");
#else // x86_64 assumed
    static_assert(NUM_FP_ARGS <= 8 && NUM_INT_ARGS <= 8, "Stack parameters are not yet supported for x86_64");
#endif

    assert_invariants();

    // reset the amount of memory we've "used" in the mmaped section,
    // as we're preparing to enter a new function call
    mmaped_used_amt_ = 0;

    // prepare return location
    TRY(setup_function_return());

    // prepare arguments
    user_regs_struct int_regs = TRY(get_registers());
    user_fpregs_struct fp_regs = TRY(get_fp_registers());

    // unused in the case that there are no function arguments
    [[maybe_unused]] auto setup_raw_arg = [&, num_fp = 0, num_int = 0]<typename T>(T arg) mutable {
        if constexpr (std::floating_point<T>) {
#ifdef __aarch64__
            static_assert(!std::floating_point<T>, "Floating point parameters not yet supported for aarch64");
#else // x86_64 assumed
            std::memcpy(&fp_regs.xmm_space[static_cast<std::ptrdiff_t>(num_fp * 4)], &arg, sizeof(T));
#endif
            num_fp++;
        } else {
#ifdef __aarch64__
            int_regs.regs[num_int] = arg;
#else // x86_64 assumed
            switch (num_int) {
            case 0:
                int_regs.rdi = arg;
                break;
            case 1:
                int_regs.rsi = arg;
                break;
            case 2:
                int_regs.rdx = arg;
                break;
            case 3:
                int_regs.rcx = arg;
                break;
            case 4:
                int_regs.r8 = arg;
                break;
            case 5:
                int_regs.r9 = arg;
                break;
            default:
                unreachable();
            }
#endif
            num_int++;
        }
    };

    if constexpr (sizeof...(Args) > 0) {
        std::tuple reg_param_vals = std::make_tuple(setup_function_param(std::forward<Args>(args))...);

        std::optional first_error = tuple_find_first([](const auto& val) { return val.has_error(); }, reg_param_vals);

        if (first_error.has_value()) {
            return std::visit([](const auto& err_val) { return err_val.error(); }, *first_error);
        }

        std::apply([&](const auto&... vals) { (setup_raw_arg(*vals), ...); }, reg_param_vals);

        TRY(set_registers(int_regs));
        TRY(set_fp_registers(fp_regs));
    }

    return {};
}

template <typename Arg>
Result<u64> Tracer::setup_function_param(const Arg& arg) {
    if constexpr (std::integral<Arg>) {
        return static_cast<u64>(arg);
    } else if constexpr (std::is_pointer_v<Arg>) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<u64>(arg);
    } else {
        std::uintptr_t loc = mmaped_address_ + mmaped_used_amt_;
        std::size_t num_bytes = TRY(memory_io_->write(loc, arg));

        mmaped_used_amt_ += num_bytes;

        return loc;
    }
}

template <std::floating_point Arg>
auto setup_function_param(const Arg& arg) {
    return arg;
}

template <typename Ret>
Result<Ret> Tracer::process_function_ret() {
    static_assert(std::is_fundamental_v<Ret> || std::is_pointer_v<Ret>,
                  "Non-fundamental and non-pointer types are not yet supported as function return types");

    if constexpr (std::floating_point<Ret>) {
        user_fpregs_struct fp_regs = TRY(get_fp_registers());
#ifdef __aarch64__
        UNIMPLEMENTED("TODO: implement floating-point return types for aarch64");
#else // x86_64 assumed

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<Ret*>(&fp_regs.xmm_space[0]);
#endif
    } else {
        user_regs_struct int_regs = TRY(get_registers());
#ifdef __aarch64__
        u64 ret = int_regs.regs[0];
#else // x86_64 assumed
        u64 ret = int_regs.rax;
#endif

        if constexpr (std::integral<Ret>) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return *reinterpret_cast<Ret*>(&ret);
        } else if constexpr (std::is_pointer_v<Ret>) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
            return reinterpret_cast<Ret>(ret);
        }
    }
}

} // namespace asmgrader
