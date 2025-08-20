#include "common/terminal_checks.hpp"

#include <range/v3/algorithm/any_of.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

// Determine if the terminal supports colors
// Based on: https://github.com/gabime/spdlog
// Which is subsequently based on: https://github.com/agauniyal/rang/
bool is_color_terminal() noexcept {
    static const bool RESULT = []() {
        const char* env_colorterm_p = std::getenv("COLORTERM");
        if (env_colorterm_p != nullptr) {
            return true;
        }

        static constexpr std::array<const char*, 16> TERMS = {{"ansi", "color", "console", "cygwin", "gnome", "konsole",
                                                               "kterm", "linux", "msys", "putty", "rxvt", "screen",
                                                               "vt100", "xterm", "alacritty", "vt102"}};

        const char* env_term_p = std::getenv("TERM");

        if (env_term_p == nullptr) {
            return false;
        }

        return ranges::any_of(TERMS, [&](const char* term) { return std::strstr(env_term_p, term) != nullptr; });
    }();

    return RESULT;
}

// Determine if the terminal attached
// Based on: https://github.com/gabime/spdlog
// Which is subsequently based on: https://github.com/agauniyal/rang/
bool in_terminal(FILE* file) noexcept {
    return ::isatty(fileno(file)) != 0;
}
