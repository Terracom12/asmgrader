#pragma once

#include "common/aliases.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/algorithm/copy.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <ctime>
#include <initializer_list>
#include <span>
#include <string_view>
#include <type_traits>

#include <sys/syscall.h>

struct SyscallEntry
{
    enum class Type {
        Unused = 0,
        Int32,
        Int64,
        Uint32,
        Uint64,
        VoidPtr,
        CString,
        NTCStringArray, // Null-terminated C-string array
        TimeSpecPtr,    // for nanosleep(2)
    };

    /// Primarally for converting integer-aliases (e.g., size_t, mode_t, pid_t, etc.)
    /// to the corresponding enumerator in a platform-indepentant way.
    template <typename T>
    static constexpr Type type_of = []() {
#define TC(t, r)                                                                                                       \
    else if (std::same_as<T, t>) {                                                                                     \
        return r;                                                                                                      \
    }
        using enum Type;

        if constexpr (std::is_void_v<T>) {
            return VoidPtr;
        }
        TC(i32, Int32)
        TC(i64, Int64)
        TC(u32, Uint32)
        TC(u64, Uint64)

#undef TC
    }();

    struct Param
    {
        Type type;

        std::string_view name;
    };

    consteval SyscallEntry()
        : nr{-1} {}

    constexpr SyscallEntry(int number, std::string_view syscall_name, Type return_type,
                           std::initializer_list<Param> parameters)
        : nr{number}
        , ret_type{return_type}
        , num_params_{parameters.size()} {
        ranges::copy(syscall_name, name_buffer_.begin());
        ranges::copy(parameters, params_buffer_.begin());
    }

    static constexpr SyscallEntry unknown(int nr) {
        decltype(name_buffer_) buffer{};
        auto* end_it = fmt::format_to(buffer.data(), FMT_COMPILE("<unknown ({})>"), nr);

        *end_it = '\0';

        return {nr, Type::Unused, {}, buffer};
    }

    /// Syscall number (arch-specific)
    int nr{};

    /// Human-readable name of the syscall (e.g., openat, getpid)
    constexpr std::string_view name() const { return {name_buffer_.begin(), name_buffer_.end()}; }

    constexpr auto parameters() const { return std::span{params_buffer_.begin(), num_params_}; }

    Type ret_type{};

private:
    // No syscall name is > 128 chars in length (or even close...)
    static constexpr int MAX_NAME_LEN = 128;
    // Underlying buffer for ``name`` so that we may use fmtlib to create custom `<unknown (NR)>` names
    std::array<char, MAX_NAME_LEN> name_buffer_{};

    /// Linux syscalls do not have more than 6 parameters
    static constexpr int MAX_NUM_PARAMS = 6;
    std::array<Param, MAX_NUM_PARAMS> params_buffer_{};
    std::size_t num_params_ = 0;

    constexpr SyscallEntry(int number, Type return_type, std::array<Param, MAX_NUM_PARAMS> parameters,
                           std::array<char, MAX_NAME_LEN> name_buffer)
        : nr{number}
        , ret_type{return_type}
        , name_buffer_{name_buffer}
        , params_buffer_{parameters} {}
};

#include "subprocess/syscall_entries.inl" // IWYU pragma: export
