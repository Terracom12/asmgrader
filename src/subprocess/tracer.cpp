#include "tracer.hpp"

#include "logging.hpp"
#include "subprocess/syscall_entries.inl"
#include "subprocess/tracer_types.hpp"
#include "util/expected.hpp"
#include "util/linux.hpp"

#include <boost/range.hpp>
#include <fmt/format.h>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>

#include <linux/ptrace.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>

using namespace std::chrono_literals;

void Tracer::begin(pid_t pid) {
    pid_ = pid;

    assert_invariants();

    // Wait for SIGSTOP raised by Tracer::init_child
    auto waitid_res = TracedWaitid::waitid(P_PID, pid_);
    DEBUG_ASSERT(waitid_res.value().signal_num == SIGSTOP, "Tracer::init_child was not called in child process");
    LOG_DEBUG("Waitid returned: {}", waitid_res.value());
    // Set options:
    //   Stop tracee upon execve
    //   Deliver a info on a syscall trap (see TracedWaitid::parse or ptrace(2))
    util::linux::ptrace(PTRACE_SETOPTIONS, pid_, nullptr, PTRACE_O_TRACEEXEC | PTRACE_O_TRACESYSGOOD);

    resume_until([](TracedWaitid event) { return event.ptrace_event == PtraceEvent::Exec; });

    // FIXME: DEBUG
    util::linux::ptrace(PTRACE_CONT, pid_);
}

void Tracer::init_child() {
    // Request to be traced by parent process
    util::linux::ptrace(PTRACE_TRACEME);

    // Stop process so that parent has a chance to attach before further action
    util::linux::raise(SIGSTOP);
}

void Tracer::assert_invariants() const {
    const std::string proc_stat_pathname = fmt::format("/proc/{}/stat", pid_);

    if (const auto proc_stat_res = util::linux::stat(proc_stat_pathname); proc_stat_res.has_error()) {
        LOG_FATAL("Traced child of pid={} does not exist in procfs ({})", pid_, proc_stat_res.error());
        throw std::runtime_error("contract violation");
    }

    std::fstream proc_stat(proc_stat_pathname, std::ios::in);

    // see the proc(5) manpage
    struct ProcStatInfo
    {
        int pid;
        std::string comm;
        char state;
        int ppid;
    } info{};

    proc_stat >> info.pid >> info.comm >> info.state >> info.ppid;

    // Check for invalid states:
    //   Z   - zombie
    //   x/X - Dead
    if (info.state == 'Z' || std::tolower(info.state) == 'x') {
        LOG_FATAL("Traced child of pid={} invalid state={}", pid_, info.state);
        throw std::runtime_error("contract violation");
    }

    // Check that parent is THIS process
    const int expected_ppid = util::linux::getpid().value();
    if (info.ppid != expected_ppid) {
        LOG_FATAL("Traced child of pid={} has parent proc that is not this one (expected={}, actual ppid={})", pid_,
                  expected_ppid, info.ppid);
        throw std::runtime_error("contract violation");
    }
}

namespace {

// SyscallRecord from_syscall_info(struct ptrace_syscall_info* entry, struct ptrace_syscall_info* exit) {
//     // See ptrace(2) - PTRACE_GET_SYSCALL_INFO
//     switch (entry->op) {
//     case PTRACE_SYSCALL_INFO_ENTRY:
//     case PTRACE_SYSCALL_INFO_EXIT:
//     case PTRACE_SYSCALL_INFO_SECCOMP:
//         return {};
//     }
// }

} // namespace

SyscallRecord Tracer::get_syscall_info(struct ptrace_syscall_info* entry, struct ptrace_syscall_info* exit) const {
    assert_invariants();
    ASSERT(entry->op == PTRACE_SYSCALL_INFO_ENTRY);
    ASSERT(exit->op == PTRACE_SYSCALL_INFO_EXIT);

    decltype(SyscallRecord::args) args;

    // Get info from syscall entry
    const auto& syscall_entry = SYSCALL_MAP.at(entry->entry.nr);

    for (int i = 0; const auto& [ty, _] : syscall_entry.parameters()) {
        args.push_back(from_raw_value(entry->entry.args[i], ty));
        i++;
    }

    // Get info (retcode/error) from syscall exit
    std::int64_t ret_val = exit->exit.rval;

    auto ret = [&]() -> decltype(SyscallRecord::ret) {
        if (exit->exit.is_error) {
            return util::linux::ErrorCode{static_cast<int>(-ret_val)};
        }
        return ret_val;
    }();
}

// util::Expected<SyscallRecord, TimeoutError> Tracer::run_to_syscall() {
//
//     util::linux::ptrace(PTRACE_CONT, pid_);
//     auto res = TracedWaitid::wait_with_timeout(pid, 100ms);
//     LOG_DEBUG("Waitid returned: {}", res);
//     if (res.value().value().ptrace_event == PtraceEvent::Exec) {
//         break;
//     }
// }
