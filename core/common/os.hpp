#pragma once

// NOLINTNEXTLINE(readability-identifier-naming)
enum class ProcessorKind { Aarch64, x86_64 };

constexpr auto SYSTEM_PROCESSOR =
#if defined(__aarch64__)
#define ASMGRADER_AARCH64
    ProcessorKind::Aarch64;
#elif defined(__x86_64__)
#define ASMGRADER_X86_64
    ProcessorKind::x86_64;
#else
#error "Unsupported system architecture!"
#endif
