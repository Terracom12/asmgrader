#pragma once

#include "subprocess/syscall.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>

#include <sys/types.h>

static constexpr int NUM_SYSCALLS = 545;

static constexpr auto SYSCALL_MAP = [] {
    std::array<SyscallEntry, NUM_SYSCALLS> syscall_map{};

    std::generate(syscall_map.begin(), syscall_map.end(), [nr = 0]() mutable { return SyscallEntry::unknown(nr++); });

#define SYSENT(name, ret_type, ...) syscall_map[SYS_##name] = SyscallEntry(SYS_##name, #name, ret_type, {__VA_ARGS__})

    using enum SyscallEntry::Type;

    const auto size_type = SyscallEntry::type_of<std::size_t>;
    const auto ssize_type = SyscallEntry::type_of<ssize_t>;
    const auto mode_type = SyscallEntry::type_of<mode_t>;
    const auto off_type = SyscallEntry::type_of<off_t>;

    SYSENT(read, ssize_type, {Int32, "fd"}, {VoidPtr, "buf"}, {size_type, "count"});
    SYSENT(write, ssize_type, {Int32, "fd"}, {VoidPtr, "buf"}, {size_type, "count"});
    SYSENT(exit, Unused, {Int32, "status"});

    SYSENT(open, Int32, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});
    SYSENT(creat, Int32, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});
    SYSENT(openat, Int32, {Int32, "dirfd"}, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});

    SYSENT(open, Int32, {Int32, "fd"});

    SYSENT(mmap, VoidPtr, {VoidPtr, "addr"}, {size_type, "length"}, {Int32, "prot"}, {Int32, "flags"}, {Int32, "fd"},
           {off_type, "off_t"});
    SYSENT(munmap, VoidPtr, {VoidPtr, "addr"}, {size_type, "length"});

    SYSENT(nanosleep, Int32, {TimeSpecPtr, "req"}, {TimeSpecPtr, "rem"});

#undef SYSNAME_ELEM

    return syscall_map;
}();
