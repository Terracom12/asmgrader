#include "subprocess/memory/ptrace_memory_io.hpp"

#include "util/byte_vector.hpp"
#include "util/linux.hpp"

#include <cstddef>

#include <sys/ptrace.h>

util::Result<ByteVector> PtraceMemoryIO::read_block_impl(std::uintptr_t address, std::size_t length) {
    // TODO: Detetermine whether alignment logic is necessary

    // TODO: Use <algorithm> instead of ptr arithmetic
    ByteVector result_buffer(length);
    unsigned char* raw_buffer_ptr = reinterpret_cast<unsigned char*>(result_buffer.data());

    // NOLINTBEGIN(google-runtime-int) - based on ptrace(2) spec

    // Credit: https://unix.stackexchange.com/a/9068
    static constexpr auto ALIGNMENT = sizeof(long) - 1;

    while (length != 0) {
        size_t todo = std::min(length, sizeof(long) - (address & ALIGNMENT));
        long res = TRYE(util::linux::ptrace(PTRACE_PEEKTEXT, get_pid(), address - (address & ALIGNMENT), 0),
                        util::ErrorKind::SyscallFailure);

        std::memcpy(raw_buffer_ptr, reinterpret_cast<char*>(&res) + (address & ALIGNMENT), todo);

        raw_buffer_ptr += todo;
        address += todo;
        length -= todo;
    }

    // NOLINTEND(google-runtime-int)

    return result_buffer;
}
util::Result<void> PtraceMemoryIO::write_block_impl(std::uintptr_t address, const ByteVector& data) {
    // FIXME: Don't just extend by 0-bytes
    // TODO: Detetermine whether alignment logic is necessary
    static constexpr int ALIGNMENT = sizeof(long); // NOLINT(google-runtime-int) - based on ptrace(2) spec
    // ASSERT(/*addr % ALIGNMENT == 0 &&*/ data.size() % ALIGNMENT == 0,
    //        "'data.size()' and 'n' must be multiples of alignement - sizeof(long)");
    auto aligned_data = data;
    aligned_data.resize(aligned_data.size() + ((ALIGNMENT - (aligned_data.size() % ALIGNMENT)) % ALIGNMENT));

    auto iter = aligned_data.begin();

    for (std::size_t i = 0; i < aligned_data.size(); i += ALIGNMENT) {
        TRYE(util::linux::ptrace(PTRACE_POKETEXT, get_pid(), address, *reinterpret_cast<const long*>(&*iter)),
             util::ErrorKind::SyscallFailure);

        iter += ALIGNMENT;
        address += ALIGNMENT;
    }

    return {};
}

// TODO: Consolidate repeated code in this and other read fn
// ByteVector PtraceMemoryIO::read_until_impl(std::uintptr_t address, const std::function<bool(std::byte)>& predicate) {
//     constexpr std::size_t MAX_SIZE = 10 * std::mega::num; // 10 MB
//     constexpr std::size_t INIT_SIZE = 128;
//     // TODO: Detetermine whether alignment logic is necessary
//
//     // TODO: Use <algorithm> instead of ptr arithmetic
//     ByteVector result_buffer;
//     result_buffer.reserve(INIT_SIZE);
//
//     // Credit: https://unix.stackexchange.com/a/9068
//     static constexpr int ALIGNMENT = sizeof(long); // NOLINT(google-runtime-int) - based on ptrace(2) spec
//
//     while (result_buffer.size() < MAX_SIZE) {
//         size_t todo = sizeof(long) - (address & ALIGNMENT);
//         auto res = util::linux::ptrace(PTRACE_PEEKTEXT, get_pid(), address - (address & ALIGNMENT), 0);
//
//         if (!res) {
//             LOG_WARN("PTRACE_PEEKTEXT failed at {:#x}", address - (address & ALIGNMENT));
//             return {}; // FIXME
//         }
//
//         for (auto byte : std::bit_cast<std::array<std::byte, sizeof(long)>>(res.value())) {
//             if (predicate(byte)) {
//                 goto finish;
//             }
//
//             result_buffer.push_back(byte);
//         }
//
//         address += todo;
//     }
//
//     LOG_WARN("Buffer for `PtraceMemoryIO::read_until_impl` exceeded max size of {} bytes", MAX_SIZE);
//
// finish:
//     return result_buffer;
// }
