#include "test_context.hpp"

#include "api/asm_buffer.hpp"
#include "api/registers_state.hpp"
#include "api/test_base.hpp"
#include "common/aliases.hpp"
#include "common/byte_array.hpp"
#include "common/error_types.hpp"
#include "common/macros.hpp"
#include "common/unreachable.hpp"
#include "exceptions.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "program/program.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/syscall_record.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <gsl/util>
#include <range/v3/algorithm/count.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/poll.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <unistd.h>

TestContext::TestContext(TestBase& test, Program program,
                         std::function<void(const RequirementResult&)> on_requirement) noexcept
    : associated_test_{&test}
    , prog_{std::move(program)}
    , result_{.name = std::string{test.get_name()},
              .requirement_results = {},
              .num_passed = 0,
              .num_total = 0,
              .error = {}}
    , on_requirement_{std::move(on_requirement)} {}

TestResult TestContext::finalize() {
    result_.num_passed = static_cast<int>(
        ranges::count(result_.requirement_results, true, [](const RequirementResult& res) { return res.passed; }));
    result_.num_total = static_cast<int>(result_.requirement_results.size());

    return result_;
}

bool TestContext::require(bool condition, RequirementResult::DebugInfo debug_info) {
    return require(condition, "<no message>", debug_info);
}

bool TestContext::require(bool condition, std::string msg, RequirementResult::DebugInfo debug_info) {
    result_.requirement_results.emplace_back(/*.passed =*/condition, /*.msg =*/std::move(msg),
                                             /*.debug_info =*/debug_info);

    on_requirement_(result_.requirement_results.back());

    return condition;
}

std::string_view TestContext::get_name() const {
    return associated_test_->get_name();
}

std::string TestContext::get_stdout() {
    return TRY_OR_THROW(prog_.get_subproc().read_stdout(), "failed to read stdout");
}

std::string TestContext::get_full_stdout() {
    return prog_.get_subproc().get_full_stdout();
}

void TestContext::send_stdin(const std::string& input) {
    TRY_OR_THROW(prog_.get_subproc().send_stdin(input), "failed to write to stdin");
}

RunResult TestContext::run() {
    return TRY_OR_THROW(prog_.run(), "failed to run program");
}

void TestContext::restart_program() {
    TRY_OR_THROW(prog_.get_subproc().restart(), "failed to restart program");
}

util::Result<SyscallRecord> TestContext::exec_syscall(u64 sys_nr, std::array<std::uint64_t, 6> args) {
    return prog_.get_subproc().get_tracer().execute_syscall(sys_nr, args);
}

const std::vector<SyscallRecord>& TestContext::get_syscall_records() const {
    return prog_.get_subproc().get_tracer().get_records();
}

std::size_t TestContext::flush_stdin() {
#ifndef SYS_ppoll
#warning "Your system does not support the `ppoll` syscall! TestContext::flush_stdin will not work!"
    UNIMPLEMENTED("SYS_ppoll not defined!");
#else
    // Buffer to send SYS_read output to
    static const AsmBuffer READ_BUFFER = create_buffer<32>();
    // struct timespec buffer for ppoll
    static const AsmBuffer ZERO_TIMESPEC_BUFFER = create_buffer<sizeof(timespec)>();
    // A fd_set with only STDIN for SYS_select
    static const AsmBuffer STDIN_ONLY_POLLFD_BUFFER = create_buffer<sizeof(pollfd)>();

    ZERO_TIMESPEC_BUFFER.zero();

    // Loop until we break due to SYS_select returning 0
    for (std::size_t bytes_read = 0;;) {
        pollfd pollfd_stdin_only{};
        pollfd_stdin_only.fd = STDIN_FILENO;
        pollfd_stdin_only.events = POLLIN; // poll only for new data available to be read

        STDIN_ONLY_POLLFD_BUFFER.set_value(ByteArray<sizeof(pollfd)>::bit_cast(pollfd_stdin_only));

        // First check that stdin has any data to be read, so that we don't block
        // see ppoll(2)
        SyscallRecord ppoll_res = TRY_OR_THROW(exec_syscall(SYS_ppoll, {/*fds=*/STDIN_ONLY_POLLFD_BUFFER.get_address(),
                                                                        /*nfds=*/nfds_t{1},
                                                                        /*tmo_p=*/ZERO_TIMESPEC_BUFFER.get_address(),
                                                                        /*sigmask=*/0}),
                                               "failed to flush stdin");
        if (!ppoll_res.ret.has_value() || ppoll_res.ret->has_error()) {
            throw ContextInternalError{"failed to flush stdin"};
        }

        // 0 fds returned -> no bytes to be read
        if (ppoll_res.ret == 0) {
            LOG_DEBUG("flushed {} bytes from stdin", bytes_read);
            return bytes_read;
        }
        //
        // >0 (i.e. 1) fds returned -> there ARE bytes to be read!

        SyscallRecord read_res = TRY_OR_THROW(exec_syscall(SYS_read, {
                                                                         /*fd=*/STDIN_FILENO,
                                                                         /*buf=*/READ_BUFFER.get_address(),
                                                                         /*len=*/READ_BUFFER.size(),
                                                                     }),
                                              "failed to flush stdin");

        if (!read_res.ret.has_value()) {
            throw ContextInternalError{"failed to flush stdin"};
        }

        if (read_res.ret->has_error()) {
            throw ContextInternalError{util::ErrorKind::SyscallFailure, "failed to flush stdin"};
        }

        bytes_read += gsl::narrow_cast<std::size_t>(read_res.ret->value());
    }

    unreachable();
#endif
}

RegistersState TestContext::get_registers() const {
    auto regs = TRY_OR_THROW(prog_.get_subproc().get_tracer().get_registers(), "failed to get registers");
    auto fp_regs = TRY_OR_THROW(prog_.get_subproc().get_tracer().get_fp_registers(), "failed to get registers");

    return RegistersState::from(regs, fp_regs);
}
