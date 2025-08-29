#pragma once

#ifdef __cpp_lib_unreachable
#include <utility>
using std::unreachable;
#else

namespace asmgrader {

// Example implemention from cppreference
[[noreturn]] inline void unreachable() {
    __builtin_unreachable();
}

} // namespace asmgrader

#endif
