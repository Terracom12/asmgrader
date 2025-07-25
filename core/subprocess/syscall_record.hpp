#pragma once

#include "subprocess/syscall.hpp"
#include "util/expected.hpp"

#include <optional>
#include <variant>

/// Record of a syscall for use with Tracer to keep track of which syscalls a child process invokes.
struct SyscallRecord
{
    /// An arbitrary syscall argument
    /// void* is a catch-all for any pointer type
    using SyscallArg = std::variant<std::int32_t, std::int64_t, std::uint32_t, std::uint64_t, std::string,
                                    std::vector<std::string>, void*, std::timespec>;

    std::uint64_t num;
    std::vector<SyscallArg> args;
    std::optional<util::Expected<std::int64_t>> ret;

    std::uint64_t instruction_pointer;
    std::uint64_t stack_pointer;
};

template <>
struct fmt::formatter<SyscallRecord> : DebugFormatter
{

    auto format(const SyscallRecord& from, format_context& ctx) const {
        if (is_debug_format) {
            return format_to(ctx.out(),
                             "SyscallRecord{{.num = {} [SYS_{}], .args = {}, .ret = {}, .ip = 0x{:X}, .sp = 0x{:X}}}",
                             from.num, SYSCALL_MAP.at(from.num).name(), from.args, from.ret, from.instruction_pointer,
                             from.stack_pointer);
        }
        UNIMPLEMENTED("Use debug format spec `{:?}` for SyscallRecord");
    }
};

#include "util/timespec_operator_eq.hpp" // IWYU pragma: keep; To permit default comparison of `SyscallRecord::SyscallArg`
