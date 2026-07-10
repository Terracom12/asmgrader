#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/class_traits.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/linux.hpp>

#include <fmt/format.h>
#include <gsl/util>
#include <libassert/assert.hpp>

#include <chrono>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

namespace asmgrader {

class Subprocess : NonCopyable
{
public:
    /// Creates a sub (child) process by running ``exec`` with ``args``
    /// ENV variables remain as default for for the child process.
    explicit Subprocess(std::string exec, std::vector<std::string> args);
    virtual ~Subprocess();
    Subprocess(Subprocess&&) noexcept;
    Subprocess& operator=(Subprocess&&) noexcept;

    /// Which output file descriptor to read from
    enum class WhichOutput : u8 { None = 0, Stdout = 1, Stderr = 2, StdoutAndStderr = Stdout | Stderr };

    /// stdout_str is only valid if WhichOutput::Stdout was included in the request
    /// stderr_str is only valid if WhichOutput::Stderr was included in the request
    struct OutputResult
    {
        std::string stdout_str;
        std::string stderr_str;
    };

    /// Read buffered output since the last call to this function
    OutputResult read_output(WhichOutput which = WhichOutput::StdoutAndStderr);

    /// Get all output since the program has launched
    OutputResult read_full_output(WhichOutput which = WhichOutput::StdoutAndStderr);

    /// Read log output from child since the last call to this function
    Expected<std::string> read_logs();

    /// Propegates logs to this processes default logger as by first calling ``read_logs``
    Expected<> propegate_logs();

    template <typename Rep, typename Period>
    [[deprecated]] Result<std::string> read_stdout(const std::chrono::duration<Rep, Period>& timeout) {
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();

        ASSERT(millis <= std::numeric_limits<int>::max(), "poll(2) is defined to accept an int parameter for millis");

        read_pipe_poll(stdout_, gsl::narrow_cast<int>(millis));
        return new_output(WhichOutput::Stdout).stdout_str;
    }

    /// Updates output result based on cursor positions
    /// If the cursor is located before the end of the string, then a substring is used
    /// (starting at the cursor position)
    void get_new_output(OutputResult& res);

    Result<void> send_stdin(std::string_view str) const;

    // Forks the current process to start a new subprocess as specified
    virtual Result<void> start();

    // Blocks until exit. An alias for ``wait_for_exit(A VERY LONG TIME)``
    int wait_for_exit();

    // Blocks until exit or timeout
    virtual Result<int> wait_for_exit(std::chrono::microseconds timeout);

    /// Whether child process is alive
    bool is_alive() const;

    /// Close stdout and stdin pipes to child
    /// Useful for when the child is blocking on a read to stdin
    Result<void> close_pipes();

    pid_t get_pid() const { return child_pid_; }

    std::optional<int> get_exit_code() const { return exit_code_; }

    /// Manually kill subprocess with SIGKILL
    virtual Result<void> kill();

    virtual Result<void> restart();

protected:
    // Allow derived classes to customize initialization
    Subprocess() = default;
    virtual Result<void> create(const std::string& exec, const std::vector<std::string>& args);

    virtual Result<void> init_child();
    virtual Result<void> init_parent();

private:
    pid_t child_pid_{};
    /// pipes to communicate with subprocess' stdout and stdin respectively
    /// The parent process will only make use of the write end of stdin_pipe_, and the read end of stdout_pipe_
    linux::Pipe stdin_pipe_{};

    struct OutputPipe
    {
        linux::Pipe pipe;
        std::string buffer;
        std::size_t cursor;
    };

    OutputPipe stdout_{};
    OutputPipe stderr_{};
    linux::Pipe log_pipe_{};

    /// Marks all open fds (other than 0,1,2) as FD_CLOEXEC so that they get closed in the child proc
    /// Run in the PARENT process.
    Expected<> mark_cloexec_all() const;

    /// Reads any data on the stdout pipe to stdout_buffer_
    Result<void> read_stdout_impl();

    /// Reads any immediately available data available on the specified pipe's read end
    /// Writes any new data to that OutputPipe's buffer
    /// Throws a std::logic_error if any syscalls fail
    static void read_pipe_nonblock(OutputPipe& pipe);

    /// Polls the pipe's read end for timeout_ms millis, or until available input arrives
    /// Writes any new data to that OutputPipe's buffer
    /// \returns true if a successful read occurred, false if timed out
    static bool read_pipe_poll(OutputPipe& pipe, int timeout_ms);

    /// Obtain new output based on cursor positions and buffers,
    /// and set cursors to the end of their resp. buffers
    OutputResult new_output(WhichOutput which);

    std::optional<int> exit_code_;

    std::string exec_;
    std::vector<std::string> args_;
};

// TODO: macro for bitfield enums

constexpr std::string_view format_as(const Subprocess::WhichOutput& from) {
    switch (from) {
    case Subprocess::WhichOutput::None:
        return "none";
    case Subprocess::WhichOutput::Stdout:
        return "stdout";
    case Subprocess::WhichOutput::Stderr:
        return "stderr";
    case Subprocess::WhichOutput::StdoutAndStderr:
        return "stdout&stderr";
    default:
        return "<unknown>";
    }
}

constexpr Subprocess::WhichOutput operator&(const Subprocess::WhichOutput& lhs, const Subprocess::WhichOutput& rhs) {
    return static_cast<Subprocess::WhichOutput>(fmt::underlying(lhs) & fmt::underlying(rhs));
}

constexpr Subprocess::WhichOutput operator|(const Subprocess::WhichOutput& lhs, const Subprocess::WhichOutput& rhs) {
    return static_cast<Subprocess::WhichOutput>(fmt::underlying(lhs) | fmt::underlying(rhs));
}

} // namespace asmgrader
