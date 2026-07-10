#include <asmgrader/logging.hpp>

#include <memory>

#include <spdlog/cfg/env.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace asmgrader {

void configure_logger(std::shared_ptr<spdlog::logger>& logger) {
    // #ifndef ASMGRADER_NDEBUG
    //     logger->set_level(spdlog::level::warn);
    // #else
    //     logger->set_level(spdlog::level::err);
    // #endif

#ifndef ASMGRADER_NDEBUG
    logger->set_pattern("[%T.%e] [%^%8l%$] [pid %6P] [%30!!@%20!s:%-4#] %v");
#else
    // Pattern:
    //   time - [HH:MM:SS.MS]
    //   level (colored, center aligned) - [ info ]
    //   process id - [pid 12345]
    //   message - "foo bar"
    logger->set_pattern("[%T.%e] [%^%=8l%$] [pid %6P] %v");
#endif
}

void init_default_logger() {
    // Log to stderr. See https://github.com/gabime/spdlog/wiki/FAQ#switch-the-default-logger-to-stderr
    auto default_logger = spdlog::stderr_color_st("default");
    spdlog::set_default_logger(default_logger);

    configure_logger(default_logger);

    // Override any previously set log-level with the enviornment variable SPDLOG_LEVEL, if set
    spdlog::cfg::load_env_levels("LOG_LEVEL");
}

} // namespace asmgrader
