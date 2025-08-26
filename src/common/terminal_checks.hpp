#pragma once

#include "common/expected.hpp"

#include <cstdio>

#include <sys/ioctl.h>

namespace asmgrader {

bool is_color_terminal() noexcept;
bool in_terminal(FILE* file) noexcept;

Expected<winsize> terminal_size(FILE* file) noexcept;

} // namespace asmgrader
