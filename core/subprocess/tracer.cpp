#include "tracer.hpp"

#include "logging.hpp"
#include "subprocess/memory/ptrace_memory_io.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/tracer_types.hpp"
#include "util/byte_vector.hpp"
#include "util/error_types.hpp"
#include "util/expected.hpp"
#include "util/linux.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <elf.h> // NT_PRSTATUS
#include <fcntl.h>
#include <linux/ptrace.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h> // iovec
#include <sys/user.h>
#include <sys/wait.h>

using namespace std::chrono_literals;

util::Result<void> Tracer::begin(pid_t pid) {
    pid_ = pid;

    assert_invariants();

    // TODO: Extract this
    memory_io_ = std::make_unique<PtraceMemoryIO>(pid);

    // Wait for SIGSTOP raised by Tracer::init_child
    auto waitid_res = TRYE(TracedWaitid::waitid(P_PID, static_cast<id_t>(pid_)), SyscallFailure);
    DEBUG_ASSERT(waitid_res.signal_num == SIGSTOP, "Tracer::init_child was not called in child process");
    LOG_DEBUG("Waitid returned: {}", waitid_res);
    // Set options:
    //   Stop tracee upon execve
    //   Deliver a info on a syscall trap (see TracedWaitid::parse or ptrace(2))
    TRYE(util::linux::ptrace(PTRACE_SETOPTIONS, pid_, NULL,
                             PTRACE_O_TRACEEXEC | PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL),
         SyscallFailure);

    TRY(resume_until([](TracedWaitid event) { return event.ptrace_event == PtraceEvent::Exec; }));

    // TODO: Figure out exactly what's going on here
    // I think a syscall-exit occurs after PTRACE_EXEC event
    TRY(resume_until([](TracedWaitid wait_res) { return wait_res.is_syscall_trap; }, DEFAULT_TIMEOUT, PTRACE_SYSCALL));

    // Set up mmap to give some anonymous anywhere on the system of size PAGE_SIZE
    // (4096). This is to be used for some instructions to not overwrite code
    // in the child process
    auto mmap_syscall_res = execute_syscall(SYS_mmap, {/*addr=*/0,
                                                       /*length=*/MMAP_LENGTH,
                                                       /*prot=*/PROT_READ | PROT_EXEC,
                                                       /*flags=*/MAP_PRIVATE | MAP_ANONYMOUS,
                                                       // NOLINTNEXTLINE(google-runtime-int)
                                                       /*fd=*/static_cast<unsigned long>(-1),
                                                       /*offset=*/0});
    if (!*mmap_syscall_res.ret) {
        LOG_ERROR("mmap syscall failed in child process");
        return util::ErrorKind::SyscallFailure;
    }

    mmaped_address_ = static_cast<std::uint64_t>(mmap_syscall_res.ret->value());

    LOG_DEBUG("mmaped address: {:#X}", mmaped_address_);

    return {};
}

util::Result<void> Tracer::init_child() {
    // Request to be traced by parent process
    TRYE(util::linux::ptrace(PTRACE_TRACEME), SyscallFailure);

    // Stop process so that parent has a chance to attach before further action
    TRYE(util::linux::raise(SIGSTOP), SyscallFailure);

    return {};
}

// TODO: Consider disabling this function in release mode
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

SyscallRecord Tracer::get_syscall_entry_info(struct ptrace_syscall_info* entry) const {
    assert_invariants();

    ASSERT(entry->op == PTRACE_SYSCALL_INFO_ENTRY);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) : necessary as return from ptrace(2)

    decltype(SyscallRecord::args) args;

    // Get info from syscall entry
    const auto& syscall_entry = SYSCALL_MAP.at(entry->entry.nr);

    for (const auto& [reg_value, syscall_param] : ranges::views::zip(entry->entry.args, syscall_entry.parameters())) {
        args.push_back(from_syscall_value(reg_value, syscall_param.type));
    }

    return {.num = entry->entry.nr,
            .args = std::move(args),
            .ret = std::nullopt,
            .instruction_pointer = entry->instruction_pointer,
            .stack_pointer = entry->stack_pointer};

    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
}
void Tracer::get_syscall_exit_info(SyscallRecord& rec, struct ptrace_syscall_info* exit) const {
    assert_invariants();

    ASSERT(exit->op == PTRACE_SYSCALL_INFO_EXIT);

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) : necessary as return from ptrace(2)

    // Get info (retcode/error) from syscall exit
    std::int64_t ret_val = exit->exit.rval;

    rec.ret = [&]() -> decltype(SyscallRecord::ret) {
        if (exit->exit.is_error) {
            return util::linux::make_error_code(static_cast<int>(-ret_val));
        }
        return ret_val;
    }();

    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
}

void Tracer::jump_to(std::uintptr_t address) {
    auto regs = get_registers();
#ifdef __aarch64__
    regs.pc = address;
#else
    regs.rip = address;
#endif
    set_registers(regs);
}

user_regs_struct Tracer::get_registers() const {
    user_regs_struct result{};

    iovec iov = {.iov_base = &result, .iov_len = sizeof(result)};

    assert_expected(util::linux::ptrace(PTRACE_GETREGSET, pid_, NT_PRSTATUS, &iov));

    return result;
}

void Tracer::set_registers(user_regs_struct regs) const {
    iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

    assert_expected(util::linux::ptrace(PTRACE_SETREGSET, pid_, NT_PRSTATUS, &iov));
}

user_fpregs_struct Tracer::get_fp_registers() const {
    user_fpregs_struct result{};

    iovec iov = {.iov_base = &result, .iov_len = sizeof(result)};

    assert_expected(util::linux::ptrace(PTRACE_GETREGSET, pid_, NT_FPREGSET, &iov));

    return result;
}

void Tracer::set_fp_registers(user_fpregs_struct regs) const {
    iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

    assert_expected(util::linux::ptrace(PTRACE_SETREGSET, pid_, NT_FPREGSET, &iov));
}

SyscallRecord Tracer::execute_syscall(std::uint64_t sys_nr, std::array<std::uint64_t, 6> args) {
    auto orig_regs = get_registers();
    auto new_regs = orig_regs;

    // NOLINTBEGIN(readability-magic-numbers) : they're instr opcodes, hex is alright as long as there are comments

    // See syscall(2) for arguments order and registers
#ifdef __aarch64__
    // syscall args are x0-x5
    ranges::copy(args, new_regs.regs);
    // syscall nr is x8
    new_regs.regs[8] = sys_nr;
    std::uint64_t instr_ptr = orig_regs.pc;

    auto orig_instrs = memory_io_->read_bytes(orig_regs.pc, 8);

    // Encoding for:
    //   svc 0         - d4000001
    //   nop           - d503201f
    ByteVector new_instrs = ByteVector::from<std::uint32_t, std::uint32_t>(0xD4000001, //
                                                                           0xD503201F);
    memory_io_->write(orig_regs.pc, new_instrs);
#else
    // syscall args are rdi, rsi, rdx, r10, r8, r9
    new_regs.rdi = args[0];
    new_regs.rsi = args[1];
    new_regs.rdx = args[2];
    new_regs.r10 = args[3];
    new_regs.r8 = args[4];
    new_regs.r9 = args[5];
    // syscall nr is rax
    new_regs.rax = sys_nr;

    auto orig_instrs = memory_io_->read_bytes(orig_regs.rip, 8);

    // Encoding for:
    //   syscall - 0f05
    ByteVector new_instrs = {0x0F, 0x05};
    new_instrs.resize(8); // To keep proper alignment

    memory_io_->write(orig_regs.rip, new_instrs);
#endif

    // NOLINTEND(readability-magic-numbers)

    set_registers(new_regs);

    auto result = run_next_syscall();
    assert_expected(result, "Should not be possible to timeout when a syscall is the next instruction");

    // Restore original instructions and program state
    set_registers(orig_regs);

#ifdef __aarch64__
    memory_io_->write(orig_regs.pc, orig_instrs);
#else
    memory_io_->write(orig_regs.rip, orig_instrs);
#endif

    return result.value();
}

// TODO: Refactor to use Exxpected result values
util::Result<RunResult> Tracer::run() {
    assert_invariants();

    for (;;) {
        assert_expected(util::linux::ptrace(PTRACE_SYSCALL, pid_), "PTRACE_SYSCALL failed");

        auto wait_result = TracedWaitid::wait_with_timeout(pid_, DEFAULT_TIMEOUT);

        // FIXME: CATCH SEGFAULTS!!!

        if (wait_result == util::ErrorKind::TimedOut) {
            LOG_DEBUG("Child process (pid={}) timed out. Stopping...", pid_);

            // stop process to keep in tracable state
            TRYE(util::linux::kill(pid_, SIGSTOP), SyscallFailure);

            return util::ErrorKind::TimedOut;
        }

        if (!wait_result) {
            LOG_DEBUG("Unhandled error in child process {}", wait_result.error());
            return wait_result.error();
        }

        const TracedWaitid waitid_data = wait_result.value();

        // Handle each case for a possible return of waitid
        // For our purposes, this includes:
        //   - syscall entry
        //   - unhandled signal (e.g., SIGSEGV)
        //   - program exit
        // An unhandled event, such as a fork(2) or execve(2), will cause the this program to crash

        // encountered a syscall
        if (waitid_data.is_syscall_trap) {
            struct ptrace_syscall_info info{};

            assert_expected(util::linux::ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(info), &info));

            if (info.op == PTRACE_SYSCALL_INFO_ENTRY) {
                SyscallRecord record = get_syscall_entry_info(&info);
                syscall_records_.push_back(std::move(record));
            } else if (info.op == PTRACE_SYSCALL_INFO_EXIT) {
                if (syscall_records_.empty() || syscall_records_.back().ret != std::nullopt) {
                    LOG_DEBUG("Expected syscall entry but encountered exit. Skipping handling...");
                    continue;
                }

                get_syscall_exit_info(syscall_records_.back(), &info);
            } else {
                LOG_WARN("Unhandled syscall trap (op = {}). Skipping handling...", info.op);
            }

            continue;
        }

        // trapped by a signal (such as by a SEGFAULT)
        if (waitid_data.type == CLD_TRAPPED) {
            return RunResult::make_signal_caught(*waitid_data.signal_num);
        }

        // program exiting
        if (waitid_data.type == CLD_EXITED) {
            exit_code_ = waitid_data.exit_code;
            LOG_DEBUG("waitid got exit code = {}", exit_code_);

            return RunResult::make_exited(waitid_data.exit_code.value());
        }
    }

    __builtin_unreachable();
}

void Tracer::setup_function_return() {
    // TODO: Could do a couple fewer context switches by doing register setup all at once if perf is a concern
    user_regs_struct regs = get_registers();

    const std::uintptr_t return_loc = mmaped_address_ + mmaped_used_amt_;

    // NOLINTBEGIN(readability-magic-numbers)
#ifdef __aarch64__
    // LR
    regs.regs[30] = return_loc;

    // Encoding for:
    //   brk 0x1234    - d4224680
    //   nop           - d503201f
    // 32-bit alignment is required
    ByteVector instrs = {0x80, 0x46, 0x22, 0xD4, //
                         0x1F, 0x20, 0x03, 0xD5};

#else // x86_64 assumed
      // return address placed on stack
    regs.rsp -= 8; // FIXME: Should it not be 16-byte aligned?
    memory_io_->write(regs.rsp, return_loc);

    // Encoding for:
    //   int3 - 0xcc
    //   nop  - 0x90
    ByteVector instrs = {0xCC, 0x90, 0x90, 0x90, //
                         0x90, 0x90, 0x90, 0x90};
#endif
    // NOLINTEND(readability-magic-numbers)

    memory_io_->write(return_loc, instrs);
    mmaped_used_amt_ += instrs.size();

    set_registers(regs);
}

// FIXME: Not a great function... Does not permit handling of unexpected cases (e.g., program exiting, SIGSEGV,
// etc.)
util::Result<TracedWaitid> Tracer::resume_until(const std::function<bool(TracedWaitid)>& pred,
                                                std::chrono::microseconds timeout, int ptrace_request) const {

    assert_invariants();

    using namespace std::chrono_literals;
    using std::chrono::steady_clock;

    const auto start_time = steady_clock::now();
    std::common_type_t<decltype(start_time - start_time), std::chrono::microseconds> remaining_time = timeout;

    while (remaining_time > 0us) {
        auto ptrace_result = util::linux::ptrace(ptrace_request, pid_);
        assert_expected(ptrace_result, "ptrace failed in `resume_until`");

        auto wait_result = TracedWaitid::wait_with_timeout(pid_, timeout);

        LOG_TRACE("waitid returned: {}", wait_result);

        if (!wait_result || pred(wait_result.value())) {
            return wait_result;
        }

        const auto total_elapsed_time = steady_clock::now() - start_time;
        remaining_time = timeout - total_elapsed_time;
    }

    LOG_DEBUG("resume_until timed out at {}", timeout);
    return util::ErrorKind::TimedOut;
}

// FIXME: Not a great function... Does not permit handling of unexpected cases (e.g., program exiting, SIGSEGV,
// etc.)
util::Result<SyscallRecord> Tracer::run_next_syscall(std::chrono::microseconds timeout) const {
    assert_invariants();

    using Hrc = std::chrono::steady_clock;
    const auto start_time = Hrc::now();

    struct ptrace_syscall_info entry{};
    struct ptrace_syscall_info exit{};

    // Syscall enter
    if (auto res =
            resume_until([](TracedWaitid wait_res) { return wait_res.is_syscall_trap; }, timeout, PTRACE_SYSCALL);
        !res) {
        return res.error();
    }
    TRYE(util::linux::ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(entry), &entry), SyscallFailure);

    const auto elapsed_time = (Hrc::now() - start_time);
    timeout -= std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time);

    // Syscall exit
    if (auto res =
            resume_until([](TracedWaitid wait_res) { return wait_res.is_syscall_trap; }, timeout, PTRACE_SYSCALL);
        !res) {
        return res.error();
    }
    TRYE(util::linux::ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(exit), &exit), SyscallFailure);

    SyscallRecord rec = get_syscall_entry_info(&entry);
    get_syscall_exit_info(rec, &exit);

    return rec;
}

// FIXME: Parse fixed-length strings seperately
SyscallRecord::SyscallArg Tracer::from_syscall_value(std::uint64_t value, SyscallEntry::Type type) const {
    using enum SyscallEntry::Type;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto convert = [&]<typename T>(T /*unused*/) { return *reinterpret_cast<T*>(&value); };

    switch (type) {
    case Int32:
        return convert(std::int32_t{});
    case Int64:
        return convert(std::int64_t{});
    case Uint32:
        return convert(std::uint32_t{});
    case Uint64:
        return convert(std::uint64_t{});
    case VoidPtr:
        return convert(static_cast<void*>(nullptr));

    case CString:
        return memory_io_->read<std::string>(value);
    case NTCStringArray: {
        std::vector<std::uintptr_t> string_ptr_array =
            memory_io_->read_array<std::uintptr_t>(value, [](const auto& elem) { return elem == 0; });

        std::vector<std::string> result(string_ptr_array.size());
        ranges::transform(string_ptr_array, result.begin(),
                          [this](std::uintptr_t str_addr) { return memory_io_->read<std::string>(str_addr); });

        return result;
    }

    case TimeSpecPtr:
        return memory_io_->read<std::timespec>(value);

    default:
        ASSERT(false, "Invalid syscall entry type parse");
    }
}
