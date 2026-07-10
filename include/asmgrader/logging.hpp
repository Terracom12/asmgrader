#pragma once

#include <asmgrader/common/class_traits.hpp>
// clangd isn't good at recognizing/finding template specializations, so just including
// fmt::formatter specializations here makes my life easier, even if compile times are
// worse every time it's changed
#include <asmgrader/common/extra_formatters.hpp> // IWYU pragma: keep

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#ifndef ASMGRADER_NDEBUG
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#endif

#include <spdlog/details/console_globals.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>

// Set log level based on whether we're in DEBUG mode
// Needs to be done before including spdlog
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#include <cerrno>
#include <chrono>
#include <cstdlib> // IWYU pragma: keep; abort()
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
// #include <fmt/std.h> // FIXME: This generates errors...

#include <spdlog/cfg/env.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Wrappers for spdlog macros
#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_FATAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

/// For features that have not yet / are not planned to be implemented,
/// so that I don't bang my head against the wall in the future trying to debug something that doesn't exist
#define UNIMPLEMENTED(msg, ...)                                                                                        \
    do { /* NOLINT(cppcoreguidelines-avoid-do-while) */                                                                \
        LOG_FATAL("Feature not implemented! " msg __VA_OPT__(, ) __VA_ARGS__);                                         \
        std::abort();                                                                                                  \
    } while (false)

#ifndef ASMGRADER_NDEBUG
#define DEBUG_TIME(expr)                                                                                               \
    [&]() {                                                                                                            \
        ::asmgrader::detail::DebugTimeHelper debug_time_helper__(#expr);                                               \
        return expr;                                                                                                   \
    }()
#else
#define DEBUG_TIME(fn, ...)
#endif

namespace asmgrader {
namespace detail {

// AllowImplicitlyDeletedCopyOrMove is set to true, so the lint here seems like a false positive
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct DebugTimeHelper : NonMovable
{
    explicit DebugTimeHelper(std::string_view str)
        : str_expr(str) {
        start = std::chrono::steady_clock::now();
    }

    ~DebugTimeHelper() {
        [[maybe_unused]] const auto& stop = std::chrono::steady_clock::now();
        LOG_DEBUG("{} took {:%S}s to execute", str_expr, stop - start);
    }

    std::string_view str_expr;
    std::chrono::steady_clock::time_point start;
};

} // namespace detail

/// Obtain Linux error code message given by ``err`` via libc functions
inline std::string get_err_msg(int err) {
    return std::error_code(err, std::generic_category()).message();
}

/// Obtain Linux error (i.e., ``errno``) message via libc functions
inline std::string get_err_msg() {
    return get_err_msg(errno);
}

/// Configure the specified logger with basic settings
void configure_logger(std::shared_ptr<spdlog::logger>& logger);

void init_default_logger();

// NOLINTBEGIN(readability-identifier-naming) - Stick to spdlog conventions

/// Extremely basic sink for spdlog specifically for writing to a file descriptor
/// Boilerplate from this example: https://github.com/gabime/spdlog/wiki/Sinks#implementing-your-own-sink
template <typename Mutex>
class spdlog_fd_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit spdlog_fd_sink(int fd)
        : fd_{fd} {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {

        // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
        // msg.payload (before v1.3.0: msg.raw) contains pre formatted log

        // If needed (very likely but not mandatory), the sink formats the message before sending it to its final
        // destination:
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        write(fd_, formatted.data(), formatted.size());
    }

    void flush_() override {}

private:
    int fd_;
};

using spdlog_fd_sink_mt = spdlog_fd_sink<std::mutex>;
using spdlog_fd_sink_st = spdlog_fd_sink<spdlog::details::null_mutex>;

// NOLINTEND(readability-identifier-naming)

} // namespace asmgrader
