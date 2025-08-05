#include "test_context.hpp"

#include "grading_session.hpp"
#include "program/program.hpp"
#include "subprocess/syscall_record.hpp"
#include "test/test_base.hpp"
#include "util/unreachable.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <gsl/util>
#include <range/v3/algorithm/count.hpp>

#include <cstddef>
#include <ctime>

#include <sys/poll.h>
#include <unistd.h>

TestContext::TestContext(TestBase& test, Program program) noexcept
    : associated_test_{&test}
    , prog_{std::move(program)}
    , result_{.name = std::string{test.get_name()}, .requirement_results = {}, .num_passed = 0, .num_total = 0} {}

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
    std::string req_str;

    if (condition) {
        req_str = fmt::format("{}", fmt::styled("PASSED", fg(fmt::color::lime_green)));
    } else {
        req_str = fmt::format("{}", fmt::styled("FAILED", fg(fmt::color::red)));
    }

    LOG_DEBUG("Requirement {}: \"{}\" ({})", std::move(req_str), fmt::styled(msg, fg(fmt::color::aqua)), debug_info);

    result_.requirement_results.emplace_back(/*.passed =*/condition, /*.msg =*/std::move(msg),
                                             /*.debug_info =*/debug_info);
    return condition;
}
std::string_view TestContext::get_name() const {
    return associated_test_->get_name();
}

util::Result<std::string> TestContext::get_stdout() {
    auto out = prog_.get_subproc().read_stdout();

    if (!out) {
        return util::ErrorKind::UnknownError;
    }

    return *out;
}
std::string TestContext::get_full_stdout() {
    return prog_.get_subproc().get_full_stdout();
}

util::Result<void> TestContext::send_stdin(const std::string& input) {
    return prog_.get_subproc().send_stdin(input);
}

util::Result<RunResult> TestContext::run() {
    return prog_.run();
}

util::Result<void> TestContext::restart_program() {
    return prog_.get_subproc().restart();
}

util::Result<SyscallRecord> TestContext::exec_syscall(std::uint64_t sys_nr, std::array<std::uint64_t, 6> args) {
    return prog_.get_subproc().get_tracer().execute_syscall(sys_nr, args);
}

const std::vector<SyscallRecord>& TestContext::get_syscall_records() const {
    return prog_.get_subproc().get_tracer().get_records();
}

util::Result<std::size_t> TestContext::flush_stdin() {
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

    TRY(ZERO_TIMESPEC_BUFFER.zero());

    // Loop until we break due to SYS_select returning 0
    for (std::size_t bytes_read = 0;;) {
        pollfd pollfd_stdin_only{};
        pollfd_stdin_only.fd = STDIN_FILENO;
        pollfd_stdin_only.events = POLLIN; // poll only for new data available to be read

        TRY(STDIN_ONLY_POLLFD_BUFFER.set_value(ByteArray<sizeof(pollfd)>::bit_cast(pollfd_stdin_only)));

        // First check that stdin has any data to be read, so that we don't block
        // see ppoll(2)
        SyscallRecord ppoll_res = TRY(exec_syscall(SYS_ppoll, {/*fds=*/STDIN_ONLY_POLLFD_BUFFER.get_address(),
                                                               /*nfds=*/nfds_t{1},
                                                               /*tmo_p=*/ZERO_TIMESPEC_BUFFER.get_address(),
                                                               /*sigmask=*/0}));
        if (!ppoll_res.ret.has_value() || ppoll_res.ret->has_error()) {
            return util::ErrorKind::SyscallFailure;
        }

        // 0 fds returned -> no bytes to be read
        if (ppoll_res.ret == 0) {
            LOG_DEBUG("flushed {} bytes from stdin", bytes_read);
            return bytes_read;
        }
        //
        // >0 (i.e. 1) fds returned -> there ARE bytes to be read!

        SyscallRecord read_res = TRY(exec_syscall(SYS_read, {
                                                                /*fd=*/STDIN_FILENO,
                                                                /*buf=*/READ_BUFFER.get_address(),
                                                                /*len=*/READ_BUFFER.size(),
                                                            }));

        if (read_res.ret->has_error()) {
            return util::ErrorKind::SyscallFailure;
        }

        bytes_read += gsl::narrow_cast<std::size_t>(read_res.ret->value());
    }

    unreachable();
#endif
}
