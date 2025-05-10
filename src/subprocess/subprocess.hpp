#pragma once

#include "subprocess/tracer.hpp"
#include <chrono>
#include <string>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

class Subprocess
{
public:
    /// Creates a sub (child) process by running ``exec`` with ``args``
    /// ENV variables remain as default for for the child process.
    explicit Subprocess(const std::string& exec, const std::vector<std::string>& args);
    ~Subprocess();
    Subprocess(const Subprocess&) = delete;
    Subprocess(Subprocess&&) noexcept;
    Subprocess& operator=(const Subprocess&) = delete;
    Subprocess& operator=(Subprocess&&) noexcept;

    // TODO: Return Expected<...> type
    template <typename Rep, typename Period>
    std::string read_stdout(const std::chrono::duration<Rep, Period>& timeout) {
        return read_stdout_poll_impl(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
    }
    std::string read_stdout();

    void send_stdin(const std::string& str);

    /// Blocking
    /// Should be called before the object exits scope, unless it is known
    /// that the process has already exited.
    void wait();

    /// Whether the process is still running
    bool is_running() const;

private:
    void create(const std::string& exec, const std::vector<std::string>& args);
    std::string read_stdout_poll_impl(int timeout_ms);

    bool is_running_ = false;
    pid_t child_pid_{};
    /// pipes to communicate with subprocess' stdout and stdin respectively
    int stdin_pipefd_{};
    int stdout_pipefd_{};

    Tracer tracer_;
};
