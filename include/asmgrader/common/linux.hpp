#pragma once

#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/extra_formatters.hpp>
#include <asmgrader/logging.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/transform.hpp>

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <bits/types/siginfo_t.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace asmgrader::linux {

inline std::error_code make_error_code(int err = errno) {
    return {err, std::generic_category()};
}

/// writes to a file descriptor. See write(2)
/// returns success/failure; logs failure at debug level
inline Expected<ssize_t> write(int fd, const std::string& data) {
    ssize_t res = ::write(fd, data.data(), data.size());

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("write failed: '{}'", err);
        return err;
    }

    return res;
}

/// reads fromm a file descriptor. See read(2)
/// returns success/failure; logs failure at debug level
inline Expected<std::string> read(int fd, size_t count) { // NOLINT
    std::string buffer(count, '\0');

    ssize_t res = ::read(fd, buffer.data(), count);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("read failed: '{}'", err);
        return err;
    }

    DEBUG_ASSERT(res >= 0, "read result is negative and != -1");
    buffer.resize(static_cast<std::size_t>(res));

    return buffer;
}

/// closes a file descriptor. See close(2)
/// returns success/failure; logs failure at debug level
inline Expected<> close(int fd) {
    int res = ::close(fd);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("close failed: '{}'", err);
        return err;
    }

    return {};
}

/// see kill(2)
/// returns success/failure; logs failure at debug level
inline Expected<> kill(pid_t pid, int sig) {
    int res = ::kill(pid, sig);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("kill failed: '{}'", err);
        return err;
    }

    return {};
}

/// args and envp do NOT need to have an extra NULL element; this is added for you.
/// see execve(2)
/// returns success/failure; logs failure at debug level
inline Expected<> execve(const std::string& exec, const std::vector<std::string>& args,
                         const std::vector<std::string>& envp) {
    // Reason: execve requires non-const strings
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    std::vector<char*> cstr_arg_list(args.size() + 2, nullptr);
    std::vector<char*> cstr_envp_list(envp.size() + 1, nullptr);

    auto to_cstr = [](const std::string& str) { return const_cast<char*>(str.c_str()); };

    cstr_arg_list.front() = const_cast<char*>(exec.c_str());
    ranges::transform(args, cstr_arg_list.begin() + 1, to_cstr);
    ranges::transform(envp, cstr_envp_list.begin(), to_cstr);

    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)

    int res = ::execve(exec.data(), cstr_arg_list.data(), cstr_envp_list.data());

    auto err = make_error_code(errno);

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

/// see dup(2) and ``ForkExpected``
/// returns result from enum; logs failure at debug level
inline Expected<Fork> fork() {
    int res = ::fork();

    if (res == -1) {
        auto err = make_error_code(errno);
        LOG_DEBUG("fork failed: '{}'", err);
        return err;
    }

    if (res == 0) {
        return Fork{.which = Fork::Child, .pid = 0};
    }

    return Fork{.which = Fork::Parent, .pid = res};
}

/// see open(2)
/// returns success/failure; logs failure at debug level
inline Expected<int> open(const std::string& pathname, int flags, mode_t mode = 0) {
    // NOLINTNEXTLINE(*vararg)
    int res = ::open(pathname.c_str(), flags, mode);

    if (res == -1) {
        auto err = make_error_code(errno);
        LOG_DEBUG("open failed: '{}'", err);
        return err;
    }

    return res;
}

/// see lseek(2)
/// returns success/failure; logs failure at debug level
inline Expected<off_t> lseek(int fd, off_t offset, int whence) {
    off_t res = ::lseek(fd, offset, whence);

    if (res == -1) {
        auto err = make_error_code(errno);
        LOG_DEBUG("lseek failed: '{}'", err);
        return err;
    }

    return res;
}

/// see dup(2)
/// returns success/failure; logs failure at debug level
inline Expected<> dup2(int oldfd, int newfd) {
    int res = ::dup2(oldfd, newfd);

    if (res != newfd) {
        auto err = make_error_code(errno);

        if (res == -1) {
            LOG_DEBUG("dup2 failed: '{}'", err);
        } else {
            LOG_DEBUG("dup2 failed (INVALID RETURN CODE = {}): '{}'", res, err);
        }

        return err;
    }

    return {};
}

/// see ioctl(2)
/// returns success/failure; logs failure at debug level
// NOLINTNEXTLINE(google-runtime-int)
inline Expected<int> ioctl(int fd, unsigned long request, void* argp) {
    // NOLINTNEXTLINE(*vararg)
    int res = ::ioctl(fd, request, argp);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("ioctl failed: '{}'", err);

        return err;
    }

    return res;
}

/// see fcntl(2)
/// returns success/failure; logs failure at debug level
inline Expected<int> fcntl(int fd, int cmd, std::optional<int> arg = std::nullopt) {
    int res{};

    if (arg) {
        // NOLINTNEXTLINE(*vararg)
        res = ::fcntl(fd, cmd, arg.value());
    } else {
        // NOLINTNEXTLINE(*vararg)
        res = ::fcntl(fd, cmd);
    }

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("fcntl failed: '{}'", err);

        return err;
    }

    return Expected<int>{res};
}

/// see waitid(2)
/// returns success/failure; logs failure at debug level
inline Expected<siginfo_t> waitid(idtype_t idtype, id_t id, int options = WSTOPPED | WEXITED) {
    siginfo_t info;
    int res = ::waitid(idtype, id, &info, options);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("waitid failed: '{}'", err);

        return err;
    }

    return info;
}

/// see raise(3)
/// returns success/failure; logs failure at debug level
inline Expected<> raise(int sig) {
    int res = ::raise(sig);

    if (res == -1) {
        auto err = std::error_code(errno, std::system_category());

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
inline Expected<Pipe> pipe2(int flags = 0) {
    Pipe pipe{};

    int res = ::pipe2(&pipe.read_fd, flags);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("pipe failed: '{}'", err);

        return err;
    }

    return pipe;
}

/// see ptrace(2)
/// returns success/failure; logs failure at debug level
// NOLINTBEGIN(google-runtime-int)
//! @cond DoNotRaiseWarning
template <typename AddrT = void*, typename DataT = void*>
//! @endcond
    requires(sizeof(AddrT) <= sizeof(void*) && sizeof(DataT) <= sizeof(void*))
inline Expected<long> ptrace(int request, pid_t pid = 0, AddrT addr = NULL, DataT data = NULL) {
    //  clear errno before calling
    errno = 0;

    // Reasoning
    //   google-runtime-int                 : `long` is based on ptrace(2) spec
    //   cppcoreguidelines-pro-type-vararg  : this is a wrapper for ptrace
    //   reinterpret-cast                   : ptrace spec is `void*`, caller of this wrapper should not care
    // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-type-reinterpret-cast)
    long res = ::ptrace(static_cast<enum __ptrace_request>(request), pid, reinterpret_cast<void*>(addr),
                        reinterpret_cast<void*>(data));
    // NOLINTEND(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-type-reinterpret-cast)
    // NOLINTEND(google-runtime-int)

    // see the Return section of ptrace(2)
    if (errno) {
        auto err = make_error_code(errno);

        LOG_DEBUG("ptrace(req={}, pid={}, addr={}, data={}) failed: '{}'", request, pid, fmt_or_unknown(addr),
                  fmt_or_unknown(data), err);

        return err;
    }

    return res;
}

/// see stat(2)
inline Expected<struct ::stat> stat(const std::string& pathname) {
    struct ::stat data_result{};

    int res = ::stat(pathname.c_str(), &data_result);

    if (res == -1) {
        auto err = make_error_code(errno);

        LOG_DEBUG("stat failed: '{}'", err);

        return err;
    }

    return data_result;
}

/// see getpid(2) and getppid(2)
/// these functions "cannot fail" according to the manpage. These wrappers are provided
/// just for consistency.
inline Expected<pid_t> getpid() {
    return ::getpid();
}

inline Expected<pid_t> getppid() {
    return ::getppid();
}

/// Value type to behave as a linux signal
class Signal
{
public:
    // TODO: Consider refactor with explicit ctor and conversion

    // NOLINTNEXTLINE(google-explicit-constructor)
    Signal(int signal_num)
        : signal_num_{signal_num} {};

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator int() const { return signal_num_; }

    std::string to_string() const { return sigdescr_np(signal_num_); }

    friend std::string format_as(const Signal& from) { return from.to_string(); }

private:
    int signal_num_;
};

// TODO: Switch to using sigaction
using SignalHandlerT = void (*)(int);

inline Expected<SignalHandlerT> signal(Signal sig, SignalHandlerT handler) {
    SignalHandlerT prev_handler = ::signal(sig, handler);

    if (prev_handler == SIG_ERR) {
        auto err = make_error_code();

        LOG_DEBUG("signal failed: '{}'", err);

        return err;
    }

    return prev_handler;
}

} // namespace asmgrader::linux
