#include "subprocess.hpp"

#include "logging.hpp"
#include "subprocess/tracer_types.hpp"
#include "util/error_types.hpp"
#include "util/linux.hpp"

#include <fmt/ranges.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Subprocess::Subprocess(std::string exec, std::vector<std::string> args)
    : exec_{std::move(exec)}
    , args_{std::move(args)} {}

Subprocess::~Subprocess() {
    // if child_pid_ == 0, then initialization failed, or the object was moved from
    if (child_pid_ == 0) {
        return;
    }

    std::ignore = close_pipes();

    if (is_alive()) {
        // Try running child process to exit one last time, as it may have been blocking on open pipes
        wait_for_exit();
    }

    // TODO: should probably kill instead
}

util::Result<void> Subprocess::start() {
    return create(exec_, args_);
}

int Subprocess::wait_for_exit() {
    using namespace std::chrono_literals;

    // FIXME: if this function is called more than once, the program will probably crash due to waitid error code

    // Presumably, an 10 hours is far more than enough...
    auto res = wait_for_exit(10h);

    // Timmeout -> just crash at that point
    if (!res) {
        LOG_FATAL("Somehow, we ran this program for 10 hours... ({})", res);
        _exit(1);
    }

    return res.value();
}

util::Result<void> Subprocess::kill() {
    using namespace std::literals;

    std::ignore = close_pipes();
    TRYE(util::linux::kill(child_pid_, SIGKILL), SyscallFailure);

    auto waitid_res = TRYE(TracedWaitid::wait_with_timeout(child_pid_, 10ms), SyscallFailure);

    ASSERT(waitid_res.type == CLD_KILLED, "Waitid res after killing process should be `CLD_KILLED`");

    return {};
}

util::Result<void> Subprocess::restart() {
    if (is_alive()) {
        TRY(kill());
    }

    TRY(start());

    return {};
}

util::Result<int> Subprocess::wait_for_exit(std::chrono::microseconds timeout) {
    using namespace std::chrono_literals;
    using std::chrono::steady_clock;
    const auto start_time = steady_clock::now();
    std::common_type_t<decltype(start_time - start_time), decltype(timeout)> remaining_time = timeout;

    // FIXME: if this function is called more than once, the program will probably crash due to waitid error code

    while (remaining_time > 0us) {
        auto waitid_res = TracedWaitid::wait_with_timeout(child_pid_, timeout);

        if (!waitid_res) {
            return waitid_res.error();
        }

        if (waitid_res.value().exit_code) {
            exit_code_ = waitid_res.value().exit_code;
            return exit_code_.value();
        }

        // Check for other types of process-ending failures
        if (waitid_res.value().type == CLD_KILLED || waitid_res.value().type == CLD_DUMPED) {
            return -1;
        }
    }

    return util::ErrorKind::TimedOut;
}

util::Result<void> Subprocess::close_pipes() {
    // Make sure all available data is read before pipes are closed
    std::ignore = read_stdout_impl();

    if (stdin_pipe_.write_fd != -1) {
        TRYE(util::linux::close(stdin_pipe_.write_fd), SyscallFailure);
        stdin_pipe_.write_fd = -1;
    }
    if (stdout_pipe_.read_fd != -1) {
        TRYE(util::linux::close(stdout_pipe_.read_fd), SyscallFailure);
        stdout_pipe_.read_fd = -1;
    }

    return {};
}

Subprocess::Subprocess(Subprocess&& other) noexcept
    : child_pid_{std::exchange(other.child_pid_, 0)}
    , stdin_pipe_{std::exchange(other.stdin_pipe_, {})}
    , stdout_pipe_{std::exchange(other.stdout_pipe_, {})}
    , stdout_buffer_{std::exchange(other.stdout_buffer_, {})}
    , stdout_cursor_{std::exchange(other.stdout_cursor_, 0)} {}

Subprocess& Subprocess::operator=(Subprocess&& rhs) noexcept {
    child_pid_ = std::exchange(rhs.child_pid_, 0);
    stdin_pipe_ = std::exchange(rhs.stdin_pipe_, {});
    stdout_pipe_ = std::exchange(rhs.stdout_pipe_, {});
    stdout_buffer_ = std::exchange(rhs.stdout_buffer_, {});
    stdout_cursor_ = std::exchange(rhs.stdout_cursor_, 0);

    return *this;
}

bool Subprocess::is_alive() const {
    return util::linux::kill(child_pid_, 0) != std::make_error_code(std::errc::no_such_process);
}

std::optional<std::string> Subprocess::read_stdout_poll_impl(int timeout_ms) {
    // If the pipe is already closed, all we can do is try reading from the buffer
    if (stdout_pipe_.read_fd == -1) {
        return read_stdout();
    }

    struct pollfd poll_struct = {.fd = stdout_pipe_.read_fd, .events = POLLIN, .revents = 0};

    int res = poll(&poll_struct, 1, timeout_ms);
    // Error
    if (res == -1) {
        LOG_WARN("Error polling for read from stdout pipe: '{}'", get_err_msg());
        return "";
    }
    // Timeout occured
    if (res == 0) {
        return "";
    }

    return read_stdout();
}

std::optional<std::string> Subprocess::read_stdout() {
    std::ignore = read_stdout_impl();

    // Cursor is still at the end of the buffer -> no data was read
    if (stdout_cursor_ == stdout_buffer_.size()) {
        return std::nullopt;
    }

    auto res = stdout_buffer_.substr(stdout_cursor_);
    stdout_cursor_ = stdout_buffer_.size();

    return res;
}

const std::string& Subprocess::get_full_stdout() {
    std::ignore = read_stdout_impl();

    return stdout_buffer_;
}

util::Result<void> Subprocess::read_stdout_impl() {
    std::size_t num_bytes_avail = 0;

    if (stdout_pipe_.read_fd == -1) {
        return {};
    }

    TRYE(util::linux::ioctl(stdout_pipe_.read_fd, FIONREAD, &num_bytes_avail), SyscallFailure);

    LOG_DEBUG("{} bytes available from stdout_pipe", num_bytes_avail);

    if (num_bytes_avail == 0) {
        return {};
    }

    std::string res = TRYE(util::linux::read(stdout_pipe_.read_fd, num_bytes_avail), SyscallFailure);

    stdout_buffer_ += res;

    return {};
}

util::Result<void> Subprocess::send_stdin(const std::string& str) {
    // TODO: more abstract write wrapper that ensures all bytes were sent
    TRYE(util::linux::write(stdin_pipe_.write_fd, str), SyscallFailure);

    return {};
}

util::Result<void> Subprocess::create(const std::string& exec, const std::vector<std::string>& args) {
    stdout_pipe_ = TRYE(util::linux::pipe2(), SyscallFailure);
    stdin_pipe_ = TRYE(util::linux::pipe2(), SyscallFailure);

    util::linux::Fork fork_res = TRYE(util::linux::fork(), SyscallFailure);

    // Child process
    if (fork_res.which == util::linux::Fork::Child) {
        TRY(init_child());
        auto execve_res = util::linux::execve(exec, args, {});

        LOG_FATAL("execve failed in child proc: {}", execve_res);

        return util::ErrorKind::SyscallFailure;
    }

    // Parent process
    child_pid_ = fork_res.pid;

    return init_parent();
}

util::Result<void> Subprocess::init_child() {
    TRYE(util::linux::dup2(stdin_pipe_.read_fd, STDIN_FILENO), SyscallFailure);
    TRYE(util::linux::dup2(stdout_pipe_.write_fd, STDOUT_FILENO), SyscallFailure);

    // Close the pipe ends not being used in the child proc
    //  - read end for stdout
    //  - write end for stdin
    TRYE(util::linux::close(stdin_pipe_.write_fd), SyscallFailure);
    TRYE(util::linux::close(stdout_pipe_.read_fd), SyscallFailure);

    return {};
}

util::Result<void> Subprocess::init_parent() {
    // Close the pipe ends being used in the parent proc
    //  - write end for stdout
    //  - read end for stdin
    TRYE(util::linux::close(stdin_pipe_.read_fd), SyscallFailure);
    TRYE(util::linux::close(stdout_pipe_.write_fd), SyscallFailure);
    // stdin_pipefd_ = stdin_pipe.write_fd;  // write end of stdin pipe
    // stdout_pipefd_ = stdout_pipe.read_fd; // read end of stdout pipe

    // Make reading from stdout non-blocking
    int pre_flags = TRYE(util::linux::fcntl(stdout_pipe_.read_fd, F_GETFL), SyscallFailure);

    TRYE(util::linux::fcntl(stdout_pipe_.read_fd, F_SETFL, pre_flags | O_NONBLOCK), // NOLINT
         SyscallFailure);

    return {};
}
