#pragma once

// Set log level based on whether we're in DEBUG mode
// Needs to be done before including spdlog
#include <cstdio>
#include <cstdlib> // For abort
#include <string>
#include <system_error>

#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#elif defined(RELEASE)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Wrappers for spdlog macros
#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_FATAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

// Basic assertion macros debugging
/// Usage:
///   DEBUG_ASSERT(x == 1, "Unexpected value for {}", "x")
#define ASSERT(expr, ...)                                                                                              \
    do { /* NOLINT(cppcoreguidelines-avoid-do-while) */                                                                \
        const auto _expr_res_ = expr;                                                                                  \
        if (!_expr_res_) {                                                                                             \
            LOG_FATAL("Assertion failed (" #expr ") - " __VA_ARGS__);                                                  \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (false)
#ifdef DEBUG
#define DEBUG_ASSERT(expr, ...) ASSERT(expr, "[!DEBUG!] " __VA_ARGS__)
#else
#define DEBUG_ASSERT(expr, ...)
#endif

/// Obtain Linux error code message given by ``err`` via libc functions
inline std::string get_err_msg(int err) {
    return std::error_code(err, std::generic_category()).message();
}
/// Obtain Linux error (i.e., ``errno``) message via libc functions
inline std::string get_err_msg() {
    return get_err_msg(errno);
}

const inline bool LOGGER_INIT = []() {
    try {
#ifdef DEBUG
        spdlog::set_pattern("[%T.%e] [%^%8l%$] [pid %6P] [%30!!@\t%20s:%-4#] %v");
        spdlog::set_level(spdlog::level::trace);
#elif defined(RELEASE)
        spdlog::set_level(spdlog::level::err);
#else
        spdlog::set_level(spdlog::level::info);
#endif

#ifndef DEBUG
        // Pattern:
        //   time - [HH:MM:SS.MS]
        //   level (colored, center aligned) - [ info ]
        //   process id - [pid 12345]
        //   message - "foo bar"
        spdlog::set_pattern("[%T.%e] [%^%=8l%$] [pid %6P] %v");
#endif

        // Log to stderr. See https://github.com/gabime/spdlog/wiki/FAQ#switch-the-default-logger-to-stderr
        spdlog::set_default_logger(spdlog::stderr_color_st("default"));
    } catch (std::exception& ex) {
        (void)std::fprintf(stderr, "Failed to initialize spdlog! (%s)\n", ex.what()); // NOLINT(*vararg)
        _exit(1);
    }

    return true;
}();
