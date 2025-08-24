#pragma once

#include <cstdio>

namespace asmgrader {

bool is_color_terminal() noexcept;
bool in_terminal(FILE* file) noexcept;

} // namespace asmgrader
