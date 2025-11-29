#include <asmgrader/subprocess/subprocess.hpp>

#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/linux.hpp>
#include <asmgrader/logging.hpp>
#include <asmgrader/subprocess/tracer_types.hpp>

#include <fmt/ranges.h>
#include <libassert/assert.hpp>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace asmgrader {

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

Result<void> Subprocess::start() {
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

Result<void> Subprocess::kill() {
    using namespace std::literals;

    std::ignore = close_pipes();
    TRYE(linux::kill(child_pid_, SIGKILL), SyscallFailure);

    auto waitid_res = TRYE(TracedWaitid::wait_with_timeout(child_pid_, 10ms), SyscallFailure);

    ASSERT(waitid_res.type == CLD_KILLED, "Waitid res after killing process should be `CLD_KILLED`");

    return {};
}

Result<void> Subprocess::restart() {
    if (is_alive()) {
        TRY(kill());
    }

    // Clear all old stdout+stderr records
    stdout_.buffer.clear();
    stdout_.cursor = 0;
    stderr_.buffer.clear();
    stderr_.cursor = 0;

    TRY(start());

    return {};
}

Result<int> Subprocess::wait_for_exit(std::chrono::microseconds timeout) {
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

    return ErrorKind::TimedOut;
}

Result<void> Subprocess::close_pipes() {
    // Make sure all available data is read before pipes are closed
    read_pipe_nonblock(stdout_);
    read_pipe_nonblock(stderr_);

    if (stdin_pipe_.write_fd != -1) {
        TRYE(linux::close(stdin_pipe_.write_fd), SyscallFailure);
        stdin_pipe_.write_fd = -1;
    }
    if (stdout_.pipe.read_fd != -1) {
        TRYE(linux::close(stdout_.pipe.read_fd), SyscallFailure);
        stdout_.pipe.read_fd = -1;
    }
    if (stderr_.pipe.read_fd != -1) {
        TRYE(linux::close(stderr_.pipe.read_fd), SyscallFailure);
        stderr_.pipe.read_fd = -1;
    }

    return {};
}

Subprocess::Subprocess(Subprocess&& other) noexcept
    : child_pid_{std::exchange(other.child_pid_, 0)}
    , stdin_pipe_{std::exchange(other.stdin_pipe_, {})}
    , stdout_{std::exchange(other.stdout_, {})}
    , stderr_{std::exchange(other.stderr_, {})} {}

Subprocess& Subprocess::operator=(Subprocess&& rhs) noexcept {
    child_pid_ = std::exchange(rhs.child_pid_, 0);
    stdin_pipe_ = std::exchange(rhs.stdin_pipe_, {});
    stdout_ = std::exchange(rhs.stdout_, {});
    stderr_ = std::exchange(rhs.stderr_, {});

    return *this;
}

bool Subprocess::is_alive() const {
    return linux::kill(child_pid_, 0) != std::make_error_code(std::errc::no_such_process);
}

Subprocess::OutputResult Subprocess::read_output(WhichOutput which) {
    if ((which & WhichOutput::Stdout) != WhichOutput::None) {
        read_pipe_nonblock(stdout_);
    }
    if ((which & WhichOutput::Stderr) != WhichOutput::None) {
        read_pipe_nonblock(stderr_);
    }

    return new_output(which);
}

Subprocess::OutputResult Subprocess::read_full_output(WhichOutput which) {
    if ((which & WhichOutput::Stdout) != WhichOutput::None) {
        read_pipe_nonblock(stdout_);
    }
    if ((which & WhichOutput::Stderr) != WhichOutput::None) {
        read_pipe_nonblock(stderr_);
    }

    return OutputResult{.stdout_str = stdout_.buffer, .stderr_str = stderr_.buffer};
}

void Subprocess::read_pipe_nonblock(OutputPipe& pipe) {
    std::size_t num_bytes_avail = 0;

    if (pipe.pipe.read_fd == -1) {
        LOG_TRACE("Attempted to read from a fd that's already closed ({})", pipe.pipe.read_fd);
        return;
    }

    if (!linux::ioctl(pipe.pipe.read_fd, FIONREAD, &num_bytes_avail)) {
        throw std::logic_error("ioctl for pipe failed");
    }

    LOG_DEBUG("{} bytes available from fd ({})", num_bytes_avail, pipe.pipe.read_fd);

    if (num_bytes_avail == 0) {
        return;
    }

    if (auto res = linux::read(pipe.pipe.read_fd, num_bytes_avail)) {
        pipe.buffer += res.value();
    } else {
        throw std::logic_error("read from pipe failed");
    }
}

bool Subprocess::read_pipe_poll(Subprocess::OutputPipe& pipe, int timeout_ms) {
    struct pollfd poll_struct = {.fd = pipe.pipe.read_fd, .events = POLLIN, .revents = 0};

    // TODO: Create wrapper in linux.hpp
    int res = poll(&poll_struct, 1, timeout_ms);
    // Syscall error
    if (res == -1) {
        throw std::logic_error("poll for pipe failed");
    }
    // Timeout occured
    if (res == 0) {
        return false;
    }

    read_pipe_nonblock(pipe);

    return true;
}

Subprocess::OutputResult Subprocess::new_output(WhichOutput which) {
    OutputResult res;

    // Update res and cursor positions
    if ((which & WhichOutput::Stdout) != WhichOutput::None && stdout_.cursor < stdout_.buffer.size()) {
        res.stdout_str = stdout_.buffer.substr(stdout_.cursor);
        stdout_.cursor = stdout_.buffer.size();
    }
    if ((which & WhichOutput::Stderr) != WhichOutput::None && stderr_.cursor < stderr_.buffer.size()) {
        res.stderr_str = stderr_.buffer.substr(stderr_.cursor);
        stderr_.cursor = stderr_.buffer.size();
    }

    return res;
}

Result<void> Subprocess::send_stdin(std::string_view str) const {
    // TODO: more abstract write wrapper that ensures all bytes were sent
    TRYE(linux::write(stdin_pipe_.write_fd, str), SyscallFailure);

    return {};
}

Result<void> Subprocess::create(const std::string& exec, const std::vector<std::string>& args) {
    stdin_pipe_ = TRYE(linux::pipe2(), SyscallFailure);
    stdout_.pipe = TRYE(linux::pipe2(), SyscallFailure);
    stderr_.pipe = TRYE(linux::pipe2(), SyscallFailure);

    if (!mark_cloexec_all()) {
        LOG_WARN("Failed to set flags for fds; some fds will likely remain open in child proc");
    }

    if (!mark_cloexec_all()) {
        LOG_WARN("Failed to set flags for fds; some fds will likely remain open in child proc");
    }

    linux::Fork fork_res = TRYE(linux::fork(), SyscallFailure);

    // Child process
    if (fork_res.which == linux::Fork::Child) {
        TRY(init_child());
        auto execve_res = linux::execve(exec, args, {});

        LOG_FATAL("execve failed in child proc: {}", execve_res);

        return ErrorKind::SyscallFailure;
    }

    // Parent process
    child_pid_ = fork_res.pid;

    return init_parent();
}

Result<void> Subprocess::init_child() {
    TRYE(linux::dup2(stdin_pipe_.read_fd, STDIN_FILENO), SyscallFailure);
    TRYE(linux::dup2(stdout_.pipe.write_fd, STDOUT_FILENO), SyscallFailure);
    TRYE(linux::dup2(stderr_.pipe.write_fd, STDERR_FILENO), SyscallFailure);

    // Close the pipe ends not being used in the child proc
    //  - read end for stdout
    //  - write end for stdin and stderr
    TRYE(linux::close(stdin_pipe_.write_fd), SyscallFailure);
    TRYE(linux::close(stdout_.pipe.read_fd), SyscallFailure);
    TRYE(linux::close(stderr_.pipe.read_fd), SyscallFailure);

    namespace fs = std::filesystem;

    for (const auto& entry : fs::directory_iterator("/proc/self/fd")) {
        int fd = std::stoi(entry.path().filename().string());

        // skip stdin, stdout, stderr
        if (fd <= 2) {
            continue;
        }

        // auto res = linux::close(fd);
        //
        // // If close(2) failed for a reason other than the fd not existing, return an error
        // if (!res && res != linux::make_error_code(EBADF)) {
        //     return ErrorKind::SyscallFailure;
        // }
    }

    namespace fs = std::filesystem;

    for (const auto& entry : fs::directory_iterator("/proc/self/fd")) {
        int fd = std::stoi(entry.path().filename().string());

        // skip stdin, stdout, stderr
        if (fd <= 2) {
            continue;
        }

        // auto res = linux::close(fd);
        //
        // // If close(2) failed for a reason other than the fd not existing, return an error
        // if (!res && res != linux::make_error_code(EBADF)) {
        //     return ErrorKind::SyscallFailure;
        // }
    }

    return {};
}

Result<void> Subprocess::init_parent() {
    // Close the pipe ends being used in the parent proc
    //  - write end for stdout
    //  - read end for stdin and stderr
    TRYE(linux::close(stdin_pipe_.read_fd), SyscallFailure);
    TRYE(linux::close(stdout_.pipe.write_fd), SyscallFailure);
    TRYE(linux::close(stderr_.pipe.write_fd), SyscallFailure);
    // stdin_pipefd_ = stdin_pipe.write_fd;  // write end of stdin pipe
    // stdout_pipefd_ = stdout_pipe.read_fd; // read end of stdout pipe

    // Make reading from stdout and stderr non-blocking
    int pre_flags_stdout = TRYE(linux::fcntl(stdout_.pipe.read_fd, F_GETFL), SyscallFailure);
    int pre_flags_stderr = TRYE(linux::fcntl(stderr_.pipe.read_fd, F_GETFL), SyscallFailure);

    TRYE(linux::fcntl(stdout_.pipe.read_fd, F_SETFL, pre_flags_stdout | O_NONBLOCK), // NOLINT
         SyscallFailure);
    TRYE(linux::fcntl(stderr_.pipe.read_fd, F_SETFL, pre_flags_stderr | O_NONBLOCK), // NOLINT
         SyscallFailure);

    return {};
}

Expected<> Subprocess::mark_cloexec_all() const {
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator("/proc/self/fd")) {
        int fd = std::stoi(entry.path().filename().string());

        if (fd > 2) {
            int flags = TRY(linux::fcntl(fd, F_GETFD));
            TRY(linux::fcntl(fd, F_SETFD, flags | FD_CLOEXEC));
        }
    }

    return {};
}

} // namespace asmgrader
