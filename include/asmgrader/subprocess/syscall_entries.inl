#pragma once

// IWYU pragma: private; include "syscall.hpp"

#include <asmgrader/subprocess/syscall.hpp>

#include <range/v3/algorithm/generate.hpp>

#include <array>
#include <cstddef>
#include <cstdio>

#include <sys/syscall.h>
#include <sys/types.h>

namespace asmgrader {

static constexpr int NUM_SYSCALLS = 545;

static constexpr auto SYSCALL_MAP = [] {
    std::array<SyscallEntry, NUM_SYSCALLS> syscall_map{};

    ranges::generate(syscall_map, [nr = 0]() mutable { return SyscallEntry::unknown(nr++); });

#define SYSENT(name, ret_type, ...) syscall_map[SYS_##name] = SyscallEntry(SYS_##name, #name, ret_type, {__VA_ARGS__})

    using enum SyscallEntry::Type;

    const auto size_type = SyscallEntry::type_of<std::size_t>;
    const auto ssize_type = SyscallEntry::type_of<ssize_t>;
    const auto mode_type = SyscallEntry::type_of<mode_t>;
    const auto off_type = SyscallEntry::type_of<off_t>;

#ifdef SYS_read
    SYSENT(read, ssize_type, {Int32, "fd"}, {CString, "buf"}, {size_type, "count"});
#endif

#ifdef SYS_write
    SYSENT(write, ssize_type, {Int32, "fd"}, {CString, "buf"}, {size_type, "count"});
#endif

#ifdef SYS_exit
    SYSENT(exit, Unused, {Int32, "status"});
#endif
#ifdef SYS_exit_group
    SYSENT(exit_group, Unused, {Int32, "status"});
#endif

#ifdef SYS_open
    SYSENT(open, Int32, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});
#endif
#ifdef SYS_creat
    SYSENT(creat, Int32, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});
#endif
#ifdef SYS_openat
    SYSENT(openat, Int32, {Int32, "dirfd"}, {CString, "pathname"}, {Int32, "flags"}, {mode_type, "mode"});
#endif

#ifdef SYS_close
    SYSENT(close, Int32, {Int32, "fd"});
#endif

#ifdef SYS_mmap
    SYSENT(mmap, VoidPtr, {VoidPtr, "addr"}, {size_type, "length"}, {Int32, "prot"}, {Int32, "flags"}, {Int32, "fd"},
           {off_type, "off_t"});
#endif
#ifdef SYS_munmap
    SYSENT(munmap, VoidPtr, {VoidPtr, "addr"}, {size_type, "length"});
#endif

#ifdef SYS_nanosleep
    SYSENT(nanosleep, Int32, {TimeSpecPtr, "req"}, {TimeSpecPtr, "rem"});
#endif

#ifdef SYS_execve
    SYSENT(execve, Int32, {CString, "pathname"}, {NTCStringArray, "argv"}, {NTCStringArray, "envp"});
#endif

#undef SYSNAME_ELEM

    return syscall_map;
}();

} // namespace asmgrader
