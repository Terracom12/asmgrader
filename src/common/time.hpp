#pragma once

#include "common/expected.hpp"
#include "logging.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <string>
#include <system_error>

namespace asmgrader {

__attribute__((format(strftime, 2, 0))) // help the compiler check `format` for validity
inline Expected<std::string>
to_localtime_string(std::chrono::system_clock::time_point time_point, const char* format = "%Y-%m-%d %H:%M:%S") {

    // Convert to time_t (seconds since epoch)
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);

    // Convert to broken-down local time
    std::tm tm_buf{};

    if (localtime_r(&time, &tm_buf) != &tm_buf) {
        auto err = errno;
        LOG_WARN("localtime_r failed to convert to local time: ", get_err_msg(err));
        return std::error_code{err, std::generic_category()};
    }

    constexpr std::size_t BUF_SZ = 1024;
    std::array<char, BUF_SZ> buf{};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    // Format with strftime
    if (std::size_t num_chars = std::strftime(buf.data(), buf.size(), format, &tm_buf)) {
        return std::string{buf.begin(), num_chars};
    }
#pragma GCC diagnostic pop

    auto err = errno;
    LOG_WARN("strftime failed to format time point ", get_err_msg(err));
    return std::error_code{err, std::generic_category()};
}

} // namespace asmgrader
