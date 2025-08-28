#pragma once

#include <ctime>

// Implementing operator== for std::timespec so that the SyscallArg variant is comparable
constexpr bool operator==(const std::timespec& lhs, const std::timespec& rhs) {
    return lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec == rhs.tv_nsec;
}
