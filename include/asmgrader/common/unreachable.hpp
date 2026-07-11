#pragma once

#ifdef __cpp_lib_unreachable
#include <utility>
#endif

namespace asmgrader {

#ifdef __cpp_lib_unreachable
using std::unreachable;
#else
// Example implemention from cppreference
[[noreturn]] inline void unreachable() {
    __builtin_unreachable();
}
#endif

} // namespace asmgrader
