#pragma once

#include <boost/endian.hpp>
#include <boost/endian/detail/order.hpp>

namespace asmgrader {

// NOLINTNEXTLINE(readability-identifier-naming)
enum class ProcessorKind { Aarch64, x86_64 };

#if defined(__aarch64__)
#define ASMGRADER_AARCH64
constexpr auto SYSTEM_PROCESSOR = ProcessorKind::Aarch64;
#elif defined(__x86_64__)
#define ASMGRADER_X86_64
constexpr auto SYSTEM_PROCESSOR = ProcessorKind::x86_64;
#else
#error "Unsupported system architecture!"
#endif

enum class EndiannessKind { Little, Big };

constexpr auto ENDIANNESS =
    (boost::endian::order::native == boost::endian::order::little ? EndiannessKind::Little : EndiannessKind::Big);

} // namespace asmgrader
