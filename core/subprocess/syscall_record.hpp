#pragma once

#include "common/aliases.hpp"
#include "common/error_types.hpp"
#include "common/expected.hpp"
#include "common/formatters/debug.hpp"
#include "fmt/base.h"
#include "logging.hpp"
#include "subprocess/syscall.hpp"

#include <ctime>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace asmgrader {

/// Record of a syscall for use with Tracer to keep track of which syscalls a child process invokes.
struct SyscallRecord
{
    /// An arbitrary syscall argument
    /// void* is a catch-all for any pointer type
    using SyscallArg =
        std::variant<i32, i64, u32, u64, Result<std::string>,
                     Result<std::vector<Result<std::string>>>, void*, Result<std::timespec>>;

    u64 num;
    std::vector<SyscallArg> args;
    std::optional<Expected<i64>> ret;

    u64 instruction_pointer;
    u64 stack_pointer;
};

#include "common/timespec_operator_eq.hpp" // IWYU pragma: keep; To permit default comparison of `SyscallRecord::SyscallArg`

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::SyscallRecord> : ::asmgrader::DebugFormatter
{

    auto format(const ::asmgrader::SyscallRecord& from, format_context& ctx) const {
        if (is_debug_format) {
            return format_to(ctx.out(),
                             "SyscallRecord{{.num = {} [SYS_{}], .args = {}, .ret = {}, .ip = 0x{:X}, .sp = 0x{:X}}}",
                             from.num, ::asmgrader::SYSCALL_MAP.at(from.num).name(), from.args, from.ret,
                             from.instruction_pointer, from.stack_pointer);
        }
        UNIMPLEMENTED("Use debug format spec `{:?}` for SyscallRecord");
    }
};
