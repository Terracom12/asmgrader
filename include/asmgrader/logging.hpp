#pragma once

#include <asmgrader/common/class_traits.hpp>
// clangd isn't good at recognizing/finding template specializations, so just including
// fmt::formatter specializations here makes my life easier, even if compile times are
// worse every time it's changed
#include <asmgrader/common/extra_formatters.hpp> // IWYU pragma: keep

// Set log level based on whether we're in DEBUG mode
// Needs to be done before including spdlog
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib> // For abort
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
// #include <fmt/std.h> // FIXME: This generates errors...

#if defined(TRACE)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#elif defined(DEBUG)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#elif defined(RELEASE)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

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

#ifdef DEBUG
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
        const auto& stop = std::chrono::steady_clock::now();
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

inline void init_loggers() {
#if defined(DEBUG)
    spdlog::set_level(spdlog::level::warn);
#elif defined(RELEASE)
    spdlog::set_level(spdlog::level::err);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    // Override any previously set log-level with the enviornment variable SPDLOG_LEVEL, if set
    spdlog::cfg::load_env_levels("LOG_LEVEL");

#if defined(DEBUG) || defined(TRACE)
    spdlog::set_pattern("[%T.%e] [%^%8l%$] [pid %6P] [%30!!@%20!s:%-4#] %v");
#else
    // Pattern:
    //   time - [HH:MM:SS.MS]
    //   level (colored, center aligned) - [ info ]
    //   process id - [pid 12345]
    //   message - "foo bar"
    spdlog::set_pattern("[%T.%e] [%^%=8l%$] [pid %6P] %v");
#endif

    // Log to stderr. See https://github.com/gabime/spdlog/wiki/FAQ#switch-the-default-logger-to-stderr
    spdlog::set_default_logger(spdlog::stderr_color_st("default"));
}

} // namespace asmgrader
