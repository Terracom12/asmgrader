#pragma once

#include <asmgrader/common/aliases.hpp>

#include <libassert/assert.hpp>

#include <bit>
#include <string_view>

namespace asmgrader {

// NOLINTNEXTLINE(readability-identifier-naming)
enum class ProcessorKind { Aarch64, x86_64 };

constexpr std::string_view format_as(ProcessorKind proccessor) {
    switch (proccessor) {
    case ProcessorKind::Aarch64:
        return "Aarch64";
    case ProcessorKind::x86_64:
        return "x86_64";
    default:
        return "<unknown>";
    }
}

#if defined(__aarch64__)
#define ASMGRADER_AARCH64
constexpr auto SYSTEM_PROCESSOR = ProcessorKind::Aarch64;
#elif defined(__x86_64__)
#define ASMGRADER_X86_64
constexpr auto SYSTEM_PROCESSOR = ProcessorKind::x86_64;
#else
#error "Unsupported system architecture!"
#endif

consteval bool is_little_endian() {
    struct S
    {
        u8 msb;
        u8 lsb;
    };

    static_assert(sizeof(S) == 2, "Strange, unsupported system. struct { u8; u8; } is not packed as 2 bytes.");

    u16 one = 0x00'01;

    auto check = std::bit_cast<S>(one);

    if (check.msb == 1 && check.lsb == 0) {
        return true;
    }
    if (check.msb == 0 && check.lsb == 1) {
        return false;
    }

    UNREACHABLE("Unsupported system endianness!");
}

enum class EndiannessKind { Little = 0, Big = 1, Native = (is_little_endian() ? Little : Big) };

constexpr std::string_view format_as(EndiannessKind endianness) {
    switch (endianness) {
    case EndiannessKind::Little:
        return "little-endian";
    case EndiannessKind::Big:
        return "big-endian";
    default:
        return "<unknown>";
    }
}

} // namespace asmgrader
