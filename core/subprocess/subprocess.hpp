#pragma once

#include "util/class_traits.hpp"
#include "util/error_types.hpp"
#include "util/linux.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

class Subprocess : util::NonCopyable
{
public:
    /// Creates a sub (child) process by running ``exec`` with ``args``
    /// ENV variables remain as default for for the child process.
    explicit Subprocess(std::string exec, std::vector<std::string> args);
    virtual ~Subprocess();
    Subprocess(Subprocess&&) noexcept;
    Subprocess& operator=(Subprocess&&) noexcept;

    template <typename Rep, typename Period>
    std::optional<std::string> read_stdout(const std::chrono::duration<Rep, Period>& timeout) {
        return read_stdout_poll_impl(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
    }
    std::optional<std::string> read_stdout();

    /// Get all stdout since the program has launched
    const std::string& get_full_stdout();

    util::Result<void> send_stdin(const std::string& str);

    // Forks the current process to start a new subprocess as specified
    virtual util::Result<void> start();

    // Blocks until exit. An alias for ``wait_for_exit(A VERY LONG TIME)``
    int wait_for_exit();

    // Blocks until exit or timeout
    virtual util::Result<int> wait_for_exit(std::chrono::microseconds timeout);

    /// Whether child process is alive
    bool is_alive() const;

    /// Close stdout and stdin pipes to child
    /// Useful for when the child is blocking on a read to stdin
    util::Result<void> close_pipes();

    pid_t get_pid() const { return child_pid_; }

    std::optional<int> get_exit_code() const { return exit_code_; }

    /// Manually kill subprocess with SIGKILL
    virtual util::Result<void> kill();

    virtual util::Result<void> restart();

protected:
    // Allow derived classes to customize initialization
    Subprocess() = default;
    virtual util::Result<void> create(const std::string& exec, const std::vector<std::string>& args);

    virtual util::Result<void> init_child();
    virtual util::Result<void> init_parent();

private:
    pid_t child_pid_{};
    /// pipes to communicate with subprocess' stdout and stdin respectively
    /// The parent process will only make use of the write end of stdin_pipe_, and the read end of stdout_pipe_
    util::linux::Pipe stdin_pipe_{};
    util::linux::Pipe stdout_pipe_{};

    std::string stdout_buffer_;
    std::size_t stdout_cursor_{};

    std::optional<std::string> read_stdout_poll_impl(int timeout_ms);

    /// Reads any data on the stdout pipe to stdout_buffer_
    util::Result<void> read_stdout_impl();

    std::optional<int> exit_code_;

    std::string exec_;
    std::vector<std::string> args_;
};
