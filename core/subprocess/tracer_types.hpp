#pragma once

/// Types primarily to wrap Linux result info for use with ref``Tracer``

#include "common/class_traits.hpp"
#include "common/error_types.hpp"
#include "common/expected.hpp"
#include "common/extra_formatters.hpp"
#include "common/linux.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <chrono>
#include <optional>
#include <string>

#include <sys/ptrace.h>

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

template <>
struct fmt::formatter<PtraceEvent> : DebugFormatter
{
    auto format(const PtraceEvent& from, format_context& ctx) const {
        auto res = enum_to_str(from);

        if (res) {
            return format_to(ctx.out(), "{}", *res);
        }

        return format_to(ctx.out(), "<unknown ({})>", fmt::underlying(from));
    }

private:
    static constexpr std::optional<const char*> enum_to_str(const PtraceEvent& from) {
        using enum PtraceEvent;
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

struct TracedWaitid
{
    /// type is `si_code` in waitid(2). One of:
    ///  CLD_EXITED, CLD_KILLED, CLD_DUMPED, CLD_STOPPED, CLD_TRAPPED, CLD_CONTINUED
    int type{};

    /// Has value if and only if type == CLD_EXITED
    std::optional<int> exit_code;

    /// Has value if type is not CLD_EXITED
    std::optional<util::linux::Signal> signal_num;

    /// A PTRACE_EVENT_* value if an event was recieved
    std::optional<PtraceEvent> ptrace_event;

    /// Whether a system call trape was delivered via ptrace
    bool is_syscall_trap = false;

    static util::Expected<TracedWaitid> waitid(idtype_t idtype, id_t id, int options = WSTOPPED | WEXITED) {
        auto res = util::linux::waitid(idtype, id, options);

        if (!res) {
            return res.error();
        }

        return TracedWaitid::parse(res.value());
    }

    template <util::ChronoDuration Duration1, util::ChronoDuration Duration2 = std::chrono::microseconds>
    static util::Result<TracedWaitid> wait_with_timeout(pid_t pid, Duration1 timeout,
                                                        Duration2 poll_period = std::chrono::microseconds{1}) {
        using std::chrono::steady_clock;

        ASSERT(timeout > poll_period);

        const auto start_time = steady_clock::now();

        // while elapsed time < timeout
        while (steady_clock::now() - start_time < timeout) {
            util::Expected<siginfo_t> waitid_res = util::linux::waitid(P_PID, pid, WEXITED | WSTOPPED | WNOHANG);

            assert_expected(waitid_res);

            // si_pid will only be 0 if waitid returned early from WNOHANG
            // see waitid(2)
            if (waitid_res.value().si_pid != 0) {
                return TracedWaitid::parse(waitid_res.value());
            }

            std::this_thread::sleep_for(poll_period);
        }

        LOG_DEBUG("waitid timed out at {}", timeout);
        return util::ErrorKind::TimedOut;
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
            result.exit_code = siginfo.si_status;
            return result;
        }

        constexpr std::uint64_t SIG_MASK = 0x7f;
        constexpr std::uint64_t SYSCALL_TRAP_MASK = 0x80;
        std::uint32_t signal_bits = siginfo.si_status;

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

template <>
struct fmt::formatter<TracedWaitid> : DebugFormatter
{
    auto format(const TracedWaitid& from, format_context& ctx) const {
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
