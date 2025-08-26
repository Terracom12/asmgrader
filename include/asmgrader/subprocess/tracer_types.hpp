#pragma once

/// Types primarily to wrap Linux result info for use with ref``Tracer``

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/class_traits.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/extra_formatters.hpp>
#include <asmgrader/common/linux.hpp>
#include <asmgrader/logging.hpp>

#include <fmt/base.h>
#include <fmt/format.h>
#include <gsl/util>
#include <libassert/assert.hpp>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>

#include <bits/types/siginfo_t.h>
#include <sched.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace asmgrader {

enum class PtraceEvent {
    Stop = PTRACE_EVENT_STOP,
    Clone = PTRACE_EVENT_CLONE,
    Exec = PTRACE_EVENT_EXEC,
    Exit = PTRACE_EVENT_EXIT,
    Fork = PTRACE_EVENT_FORK,
    VFork = PTRACE_EVENT_VFORK,
    VForkDone = PTRACE_EVENT_VFORK_DONE,
    Seccomp = PTRACE_EVENT_SECCOMP
};

struct TracedWaitid
{
    /// type is `si_code` in waitid(2). One of:
    ///  CLD_EXITED, CLD_KILLED, CLD_DUMPED, CLD_STOPPED, CLD_TRAPPED, CLD_CONTINUED
    int type{};

    /// Has value if and only if type == CLD_EXITED
    std::optional<int> exit_code;

    /// Has value if type is not CLD_EXITED
    std::optional<linux::Signal> signal_num;

    /// A PTRACE_EVENT_* value if an event was recieved
    std::optional<PtraceEvent> ptrace_event;

    /// Whether a system call trape was delivered via ptrace
    bool is_syscall_trap = false;

    static Expected<TracedWaitid> waitid(idtype_t idtype, id_t id, int options = WSTOPPED | WEXITED) {
        auto res = linux::waitid(idtype, id, options);

        if (!res) {
            return res.error();
        }

        return TracedWaitid::parse(res.value());
    }

    template <ChronoDuration Duration1, ChronoDuration Duration2 = std::chrono::microseconds>
    static Result<TracedWaitid> wait_with_timeout(pid_t pid, Duration1 timeout,
                                                  Duration2 poll_period = std::chrono::microseconds{1}) {
        using std::chrono::steady_clock;

        ASSERT(timeout > poll_period);

        const auto start_time = steady_clock::now();

        // while elapsed time < timeout
        while (steady_clock::now() - start_time < timeout) {
            Expected<siginfo_t> waitid_res =
                linux::waitid(P_PID, gsl::narrow_cast<id_t>(pid), WEXITED | WSTOPPED | WNOHANG);

            ASSERT(waitid_res);

            // si_pid will only be 0 if waitid returned early from WNOHANG
            // see waitid(2)
            if (waitid_res.value().si_pid != 0) {
                return TracedWaitid::parse(waitid_res.value());
            }

            std::this_thread::sleep_for(poll_period);
        }

        LOG_DEBUG("waitid timed out at {}", timeout);
        return ErrorKind::TimedOut;
    }

    constexpr static TracedWaitid parse(const siginfo_t& siginfo) {
        TracedWaitid result{};
        result.type = siginfo.si_code;

        // Regarding ptrace(2) in conjunction with waitid(2). Firstly, for any ptrace event, a SIGTRAP will
        // have been received. Then, one of the following will be seen:
        //
        // for ptrace(2) events:
        //   (status >> 8) == (SIGTRAP | (PTRACE_EVENT_* << 8))
        //
        // for system call traps:
        //   (status >> 8) == (SIGTRAP | 0x80)
        //
        // where (status >> 8) is the same as siginfo.si_code

        if (result.type == CLD_EXITED) {
            result.exit_code = gsl::narrow_cast<int>(siginfo.si_status);
            return result;
        }

        constexpr u64 SIG_MASK = 0x7f;
        constexpr u64 SYSCALL_TRAP_MASK = 0x80;
        u32 signal_bits = gsl::narrow_cast<u32>(siginfo.si_status);

        // actual signal will be in first 7 bits
        result.signal_num = signal_bits & SIG_MASK;

        // NOT for ptrace(2) for events/syscall traps
        if (result.type != CLD_TRAPPED) {
            return result;
        }

        if ((signal_bits & SYSCALL_TRAP_MASK) != 0) {
            result.is_syscall_trap = true;
        } else if ((signal_bits >> 8) != 0) {
            result.ptrace_event = static_cast<PtraceEvent>(signal_bits >> 8);
        }

        return result;
    }
};

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::PtraceEvent> : ::asmgrader::DebugFormatter
{
    auto format(const ::asmgrader::PtraceEvent& from, format_context& ctx) const {
        auto res = enum_to_str(from);

        if (res) {
            return format_to(ctx.out(), "{}", *res);
        }

        return format_to(ctx.out(), "<unknown ({})>", fmt::underlying(from));
    }

private:
    static constexpr std::optional<const char*> enum_to_str(const ::asmgrader::PtraceEvent& from) {
        using enum ::asmgrader::PtraceEvent;
        switch (from) {
        case Stop:
            return "stop";
        case Clone:
            return "clone";
        case Exec:
            return "exec";
        case Exit:
            return "exit";
        case Fork:
            return "fork";
        case VFork:
            return "vfork";
        case VForkDone:
            return "vforkdone";
        case Seccomp:
            return "seccomp";
        default:
            return std::nullopt;
        }
    }
};

template <>
struct fmt::formatter<::asmgrader::TracedWaitid> : ::asmgrader::DebugFormatter
{
    auto format(const ::asmgrader::TracedWaitid& from, format_context& ctx) const {
        auto type_to_str = [](int wait_type) -> std::string {
            switch (wait_type) {
            case CLD_KILLED:
                return "KILLED";
            case CLD_DUMPED:
                return "DUMPED";
            case CLD_STOPPED:
                return "STOPPED";
            case CLD_CONTINUED:
                return "CONTINUED";
            case CLD_TRAPPED:
                return "TRAPPED";
            case CLD_EXITED:
                return "CONTINUED";
            default:
                return fmt::format("<unkown ({})>", wait_type);
            }
        };

        return fmt::format_to(
            ctx.out(), "TracedWaitid{{type={}, exit_code={}, signal={}, is_syscall_trap={}, ptrace_event={}}}",
            type_to_str(from.type), from.exit_code, from.signal_num, from.is_syscall_trap, from.ptrace_event);
    }
};
