#include "subprocess.hpp"
#include "logging.hpp"
#include "subprocess/tracer.hpp"
#include "util/linux.hpp"

#include <cerrno>
#include <csignal>
#include <fmt/ranges.h>

#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Subprocess::Subprocess(const std::string& exec, const std::vector<std::string>& args)
    : is_running_(true) {
    create(exec, args);
}

Subprocess::~Subprocess() {
    // if child_pid_ == 0, then initialization failed, or the object was moved from
    if (child_pid_ == 0) {
        return;
    }

    if (stdin_pipefd_ != 0) {
        util::linux::close(stdin_pipefd_);
    }
    if (stdout_pipefd_ != 0) {
        util::linux::close(stdout_pipefd_);
    }
    if (is_running_) {
        wait();
    }
}

Subprocess::Subprocess(Subprocess&& other) noexcept
    : is_running_{std::exchange(other.is_running_, false)}
    , child_pid_{std::exchange(child_pid_, 0)}
    , stdin_pipefd_{std::exchange(stdin_pipefd_, 0)}
    , stdout_pipefd_{std::exchange(stdout_pipefd_, 0)} {}

Subprocess& Subprocess::operator=(Subprocess&& rhs) noexcept {
    is_running_ = std::exchange(rhs.is_running_, false);
    child_pid_ = std::exchange(rhs.child_pid_, 0);
    stdin_pipefd_ = std::exchange(rhs.stdin_pipefd_, 0);
    stdout_pipefd_ = std::exchange(rhs.stdout_pipefd_, 0);

    return *this;
}

std::string Subprocess::read_stdout_poll_impl(int timeout_ms) {
    struct pollfd poll_struct = {.fd = stdout_pipefd_, .events = POLLIN, .revents = 0};

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
std::string Subprocess::read_stdout() {
    std::string result;

    constexpr int READ_SIZE = 1024;

    while (auto res = util::linux::read(stdout_pipefd_, READ_SIZE)) {
        if (res.value().empty()) {
            break;
        }

        result += res.value();
    }

    return result;
}

void Subprocess::send_stdin(const std::string& str) {
    util::linux::write(stdin_pipefd_, str);
}

/// Blocking
void Subprocess::wait() {
    DEBUG_ASSERT(child_pid_ > 0);
    auto info = util::linux::waitid(P_PID, static_cast<id_t>(child_pid_), WEXITED);

    LOG_INFO("Child exited with status ({})", info.transform([](siginfo_t siginfo) { return siginfo.si_status; }));

    is_running_ = false;
}

void Subprocess::create(const std::string& exec, const std::vector<std::string>& args) {
    util::linux::Pipe stdout_pipe = util::linux::pipe2().value();
    util::linux::Pipe stdin_pipe = util::linux::pipe2().value();

    util::linux::Fork fork_res = util::linux::fork().value();

    // Child process
    if (fork_res.which == util::linux::Fork::Child) {
        util::linux::dup2(stdin_pipe.read_fd, STDIN_FILENO);
        util::linux::dup2(stdout_pipe.write_fd, STDOUT_FILENO);

        // Close the pipe ends not being used in the child proc
        //  - read end for stdout
        //  - write end for stdin
        util::linux::close(stdin_pipe.write_fd);
        util::linux::close(stdout_pipe.read_fd);

        Tracer::init_child();

        util::linux::execve(exec, args, {});
    }

    // Parent process cont.
    // Close the pipe ends being used in the child proc
    //  - write end for stdout
    //  - read end for stdin
    util::linux::close(stdin_pipe.read_fd);
    util::linux::close(stdout_pipe.write_fd);
    stdin_pipefd_ = stdin_pipe.write_fd;  // write end of stdin pipe
    stdout_pipefd_ = stdout_pipe.read_fd; // read end of stdout pipe

    child_pid_ = fork_res.pid;

    // Make reading from stdout non-blocking
    auto pre_flags = util::linux::fcntl(stdout_pipefd_, F_GETFL);

    util::linux::fcntl(stdout_pipefd_, F_SETFL, pre_flags.value() | O_NONBLOCK); // NOLINT

    LOG_DEBUG("Ready to trace...");
    tracer_.begin(child_pid_);
    /*// Wait until child stops*/
    /*util::linux::waitid(P_PID, child_pid_, WSTOPPED);*/
    /*util::linux::ptrace(PTRACE_CONT, child_pid_);*/
    /**/
    /*// Wait for SIGTRAP that is by default sent upon `execve` setup by ptrace(2)*/
    /*util::linux::waitid(P_PID, child_pid_, WSTOPPED);*/
    /*util::linux::ptrace(PTRACE_CONT, child_pid_);*/
    /**/
    /*LOG_DEBUG("Start tracing...");*/
}
