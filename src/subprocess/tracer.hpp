#pragma once

#include "subprocess/syscall.hpp"
#include "subprocess/tracer_types.hpp"
#include "util/class_traits.hpp"
#include "util/expected.hpp"
#include "util/linux.hpp"

#include <concepts>
#include <ctime>
#include <fmt/format.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <linux/ptrace.h>
#include <sys/types.h> // pid_t

/// A lightweight wrapper of ptrace(2)
///
/// Instantiate and use this class in the parent process to trace a child.
/// In the child process, call Tracer::init_child.
class Tracer
{
public:
    Tracer() = default;

    /// Sets up tracing in parent process, then stops child immediately after exec call
    void begin(pid_t pid);

    /// Run the
    void resume();

    /// Set up child process for tracing
    /// Call this within the newly-forked process
    ///
    /// Immediately after a call to this function should be a call to execve.
    static void init_child();

    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds{10};

private:
    /// Ensure that invariants hold
    ///   pid_ parent is this process
    ///   pid_ state is not dead/zombie
    ///
    /// Failing this is a contract violation. Thus, the program crashes in the case
    /// that they do not hold
    void assert_invariants() const;

    /**
     * @brief Resumes the child proc until a condition is encountered, or a timeout occurs
     *
     * Blocking for up to the duration of ``timeout``.
     *
     * @tparam Func - type of ``pred``
     * @param pred [TODO:description]
     * @param request
     */
    template <typename Func, util::ChronoDuration Duration = std::chrono::milliseconds>
        requires(std::same_as<std::invoke_result_t<Func, TracedWaitid>, bool>)
    util::Expected<TracedWaitid, TimeoutError> resume_until(Func pred, Duration timeout = DEFAULT_TIMEOUT,
                                                            int ptrace_request = PTRACE_CONT) const;

    template <util::ChronoDuration Duration = std::chrono::milliseconds>
    util::Expected<SyscallRecord, TimeoutError> run_next_syscall(Duration timeout = DEFAULT_TIMEOUT) const;

    // SyscallRecord

    /// Precondition: child process must be stopped after waitid(2) returned a syscall trap event
    SyscallRecord get_syscall_info(struct ptrace_syscall_info* entry, struct ptrace_syscall_info* exit) const;

    template <typename T>
    auto from_raw_value(std::uint64_t value) const;

    SyscallRecord::SyscallArg from_raw_value(std::uint64_t value, SyscallEntry::Type type) const;

    pid_t pid_ = -1;

    std::vector<SyscallRecord> syscall_records_;
};

template <typename Func, util::ChronoDuration Duration>
    requires(std::same_as<std::invoke_result_t<Func, TracedWaitid>, bool>)
util::Expected<TracedWaitid, TimeoutError> Tracer::resume_until(Func pred, Duration timeout, int ptrace_request) const {
    assert_invariants();

    using namespace std::chrono_literals;
    using std::chrono::high_resolution_clock;

    const auto start_time = high_resolution_clock::now();
    std::common_type_t<decltype(start_time - start_time), Duration> remaining_time = timeout;

    // while elapsed time < timeout
    while (remaining_time > 0ms) {
        auto ptrace_result = util::linux::ptrace(ptrace_request, pid_);
        assert_expected(ptrace_result, "ptrace failed in `resume_until`");

        auto wait_result = TracedWaitid::wait_with_timeout(pid_, timeout);

        if (!wait_result || pred(wait_result.value())) {
            return wait_result;
        }

        const auto total_elapsed_time = high_resolution_clock::now() - start_time;
        remaining_time = timeout - total_elapsed_time;
    }

    LOG_DEBUG("resume_until timed out at {}", timeout);
    return TimeoutError{.duration = timeout, .info = "resume_until"};
}

template <util::ChronoDuration Duration>
util::Expected<SyscallRecord, TimeoutError> Tracer::run_next_syscall(Duration timeout) const {
    assert_invariants();

    using Hrc = std::chrono::high_resolution_clock;
    const auto start_time = Hrc::now();

    struct ptrace_syscall_info* entry = nullptr;
    struct ptrace_syscall_info* exit = nullptr;

    // Syscall enter
    if (auto res = resume_until([](TracedWaitid wait_res) { return wait_res.is_syscall_trap; }, timeout); !res) {
        return res;
    }
    util::assert_expected(util::linux::ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(*entry), entry));

    const auto elapsed_time = (Hrc::now() - start_time);
    timeout -= elapsed_time;

    // Syscall exit
    if (auto res = resume_until([](TracedWaitid wait_res) { return wait_res.is_syscall_trap; }, timeout); !res) {
        return res;
    }
    util::linux::ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(*exit), exit);

    return get_syscall_info(entry, exit);
}

template <typename T>
auto Tracer::from_raw_value(std::uint64_t value) const {
    if constexpr (std::is_arithmetic_v<T>) {
        // TODO: Might have to worry about endianness
        return *reinterpret_cast<T*>(&value);
    } else if constexpr (std::same_as<T, std::timespec*>) {
        // FIXME: ...
        return std::timespec{};
    } else if constexpr (std::same_as<T, std::string>) {
        // FIXME: ...
        return std::string{"UNIMPLEMENTED"};
    } else if constexpr (std::same_as<T, std::vector<std::string>>) {
        // FIXME: ...
        return std::vector<std::string>{"UNIMPLEMENTED"};
    } else if constexpr (std::same_as<T, void*>) {
        return reinterpret_cast<void*>(value);
    } else {
        static_assert(!sizeof(T), "Unkown register value type-conversion");
    }
}

inline SyscallRecord::SyscallArg Tracer::from_raw_value(std::uint64_t value, SyscallEntry::Type type) const {
    using enum SyscallEntry::Type;

    switch (type) {
    case Int32:
        return from_raw_value<std::int32_t>(value);
    case Int64:
        return from_raw_value<std::int64_t>(value);
    case Uint32:
        return from_raw_value<std::uint32_t>(value);
    case Uint64:
        return from_raw_value<std::uint64_t>(value);
    case VoidPtr:
        return from_raw_value<void*>(value);
    case CString:
        return from_raw_value<std::string>(value);
    case NTCStringArray:
        return from_raw_value<std::vector<std::string>>(value);
    case TimeSpecPtr:
        return from_raw_value<std::timespec*>(value);
    default:
        ASSERT(false, "Invalid syscall entry type parse");
    }
}
