#pragma once

#include "expected.hpp"
#include "logging.hpp"

#include <boost/range/algorithm/transform.hpp>
#include <cstdlib>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <csignal>
#include <cstddef>
#include <ctime>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace util::linux {

/// Simple wrapper around std::error_code for simplified construction and usage with Linux errors
struct ErrorCode : std::error_code
{
    explicit ErrorCode(int code) // NOLINT(*-explicit-*)
        : std::error_code{code, std::generic_category()} {};
};

/// Output formatter for ErrorCode
inline auto format_as(const ErrorCode& err) {
    return fmt::format("{} : {}", strerrorname_np(err.value()), err.message());
}

template <typename T = void>
using Result = Expected<T, ErrorCode>;

/// writes to a file descriptor. See write(2)
/// returns success/failure; logs failure at debug level
inline Result<ssize_t> write(int fd, const std::string& data) {
    ssize_t res = ::write(fd, data.data(), data.size());

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("write failed: '{}'", err);
        return err;
    }

    return res;
}

/// reads fromm a file descriptor. See read(2)
/// returns success/failure; logs failure at debug level
inline Result<std::string> read(int fd, size_t count) { // NOLINT
    std::string buffer(count, '\0');

    ssize_t res = ::read(fd, buffer.data(), count);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("read failed: '{}'", err);
        return err;
    }

    DEBUG_ASSERT(res >= 0, "read result is negative and != -1");
    buffer.resize(static_cast<std::size_t>(res));

    return buffer;
}

/// closes a file descriptor. See close(2)
/// returns success/failure; logs failure at debug level
inline Result<> close(int fd) {
    int res = ::close(fd);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("close failed: '{}'", err);
        return err;
    }

    return {};
}

/// args and envp do NOT need to have an extra NULL element; this is added for you.
/// see execve(2)
/// returns success/failure; logs failure at debug level
inline Result<> execve(const std::string& exec, const std::vector<std::string>& args,
                       const std::vector<std::string>& envp) {
    // Reason: execve requires non-const strings
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    std::vector<char*> cstr_arg_list(args.size() + 2, nullptr);
    std::vector<char*> cstr_envp_list(envp.size() + 1, nullptr);

    auto to_cstr = [](const std::string& str) { return const_cast<char*>(str.c_str()); };

    cstr_arg_list.front() = const_cast<char*>(exec.c_str());
    boost::range::transform(args, cstr_arg_list.begin() + 1, to_cstr);
    boost::range::transform(envp, cstr_envp_list.begin(), to_cstr);

    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)

    int res = ::execve(exec.data(), cstr_arg_list.data(), cstr_envp_list.data());

    auto err = ErrorCode(errno);

    if (res == -1) {
        LOG_DEBUG("execve failed: '{}'", err);
    } else {
        LOG_DEBUG("execve failed (INVALID RETURN CODE = {}): '{}'", res, err);
    }

    return err;
}

struct Fork
{
    enum { Parent, Child } which;
    pid_t pid; // Only valid if type == child
};
/// see dup(2) and ``ForkResult``
/// returns result from enum; logs failure at debug level
inline Result<Fork> fork() {
    int res = ::fork();

    if (res == -1) {
        auto err = ErrorCode(errno);
        LOG_DEBUG("fork failed: '{}'", err);
        return err;
    }

    if (res == 0) {
        return Fork{.which = Fork::Child, .pid = 0};
    }

    return Fork{.which = Fork::Parent, .pid = res};
}

/// see dup(2)
/// returns success/failure; logs failure at debug level
inline Result<> dup2(int oldfd, int newfd) {
    int res = ::dup2(oldfd, newfd);

    if (res != newfd) {
        auto err = ErrorCode(errno);

        if (res == -1) {
            LOG_DEBUG("dup2 failed: '{}'", err);
        } else {
            LOG_DEBUG("dup2 failed (INVALID RETURN CODE = {}): '{}'", res, err);
        }

        return err;
    }

    return {};
}

/// see fcntl(2)
/// returns success/failure; logs failure at debug level
inline Result<int> fcntl(int fd, int cmd, std::optional<int> arg = std::nullopt) {
    int res{};

    if (arg) {
        res = ::fcntl(fd, cmd, arg.value());
    } else {
        res = ::fcntl(fd, cmd);
    }

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("fcntl failed: '{}'", err);

        return err;
    }

    return Result<int>{res};
}

/// see waitid(2)
/// returns success/failure; logs failure at debug level
inline Result<siginfo_t> waitid(idtype_t idtype, id_t id, int options = WSTOPPED | WEXITED) {
    siginfo_t info;
    int res = ::waitid(idtype, id, &info, options);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("waitid failed: '{}'", err);

        return err;
    }

    return info;
}

/// see raise(3)
/// returns success/failure; logs failure at debug level
inline Result<> raise(int sig) {
    int res = ::raise(sig);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("raise failed: '{}'", err);

        return err;
    }

    return {};
}

struct Pipe
{
    int read_fd;
    int write_fd;
};
// Ensure that fds are packed so that pipe works properly
static_assert(offsetof(Pipe, read_fd) + sizeof(Pipe::read_fd) == offsetof(Pipe, write_fd));

/// see pipe2(2)
/// returns success/failure; logs failure at debug level
inline Result<Pipe> pipe2(int flags = 0) {
    Pipe pipe{};

    int res = ::pipe2(&pipe.read_fd, flags);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("pipe failed: '{}'", err);

        return err;
    }

    return pipe;
}

/// see ptrace(2)
/// returns success/failure; logs failure at debug level
template <typename AddrT = void*, typename DataT = void*>
inline Result<long> ptrace(int request, pid_t pid = 0, AddrT addr = NULL, DataT data = NULL) {
    // clear errno before calling
    errno = 0;

    long res = ::ptrace(static_cast<enum __ptrace_request>(request), pid, (void*)(addr), (void*)(data));

    // see the Return section of ptrace(2)
    if (errno == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("ptrace failed: '{}'", err);

        return err;
    }

    return res;
}

/// see stat(2)
inline Result<struct ::stat> stat(const std::string& pathname) {
    struct ::stat data_result{};

    int res = ::stat(pathname.c_str(), &data_result);

    if (res == -1) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("stat failed: '{}'", err);

        return err;
    }

    return data_result;
}

/// see getpid(2) and getppid(2)
/// these functions "cannot fail" according to the manpage. These wrappers are provided
/// just for consistency.
inline Result<pid_t> getpid() {
    return ::getpid();
}

inline Result<pid_t> getppid() {
    return ::getppid();
}

#define SIGSTRCASE(sig)                                                                                                \
    case sig:                                                                                                          \
        return #sig;
// Value type to behave as a linux signal
class Signal
{
public:
    Signal(int signal_num)
        : signal_num_{signal_num} {};

    operator int() const { return signal_num_; }

    std::string to_string() const {
        // TODO: Consider just converting to table

        // Signals obtained from the output of `kill -l` on bash
        switch (signal_num_) {
            SIGSTRCASE(SIGHUP)
            SIGSTRCASE(SIGINT)
            SIGSTRCASE(SIGQUIT)
            SIGSTRCASE(SIGILL)
            SIGSTRCASE(SIGTRAP)
            SIGSTRCASE(SIGABRT)
            SIGSTRCASE(SIGBUS)
            SIGSTRCASE(SIGFPE)
            SIGSTRCASE(SIGKILL)
            SIGSTRCASE(SIGSEGV)
            SIGSTRCASE(SIGPIPE)
            SIGSTRCASE(SIGALRM)
            SIGSTRCASE(SIGTERM)
            SIGSTRCASE(SIGSTKFLT)
            SIGSTRCASE(SIGCHLD)
            SIGSTRCASE(SIGCONT)
            SIGSTRCASE(SIGSTOP)
            SIGSTRCASE(SIGTSTP)
            SIGSTRCASE(SIGTTIN)
            SIGSTRCASE(SIGTTOU)
            SIGSTRCASE(SIGURG)
            SIGSTRCASE(SIGXCPU)
            SIGSTRCASE(SIGXFSZ)
            SIGSTRCASE(SIGVTALRM)
            SIGSTRCASE(SIGPROF)
            SIGSTRCASE(SIGWINCH)
            SIGSTRCASE(SIGIO)
            SIGSTRCASE(SIGPWR)
            SIGSTRCASE(SIGSYS)
        default:
            return fmt::format("<unknown ({})>", signal_num_);
        }
    }

    friend std::string format_as(const Signal& from) { return from.to_string(); }

private:
    int signal_num_;
};
#undef SIGSTRCASE

// TODO: Switch to using sigaction
using SignalHandlerT = void (*)(int);
inline Result<SignalHandlerT> signal(Signal sig, SignalHandlerT handler) {
    SignalHandlerT prev_handler = ::signal(sig, handler);

    if (prev_handler == SIG_ERR) {
        auto err = ErrorCode(errno);

        LOG_DEBUG("signal failed: '{}'", err);

        return err;
    }

    return prev_handler;
}

} // namespace util::linux
