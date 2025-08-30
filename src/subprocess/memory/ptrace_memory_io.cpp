#include "subprocess/memory/ptrace_memory_io.hpp"

#include "common/byte_vector.hpp"
#include "common/error_types.hpp"
#include "common/linux.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <sys/ptrace.h>

namespace asmgrader {

Result<ByteVector> PtraceMemoryIO::read_block_impl(std::uintptr_t address, std::size_t length) {
    // TODO: Detetermine whether alignment logic is necessary

    // TODO: Use <algorithm> instead of ptr arithmetic
    ByteVector result_buffer(length);
    auto* raw_buffer_ptr = reinterpret_cast<unsigned char*>(result_buffer.data());

    // NOLINTBEGIN(google-runtime-int) - based on ptrace(2) spec

    // Credit: https://unix.stackexchange.com/a/9068
    static constexpr auto alignment = sizeof(long) - 1;

    while (length != 0) {
        std::size_t todo = std::min(length, sizeof(long) - (address & alignment));
        long res = TRYE(linux::ptrace(PTRACE_PEEKTEXT, get_pid(), address - (address & alignment), 0),
                        ErrorKind::SyscallFailure);

        std::memcpy(raw_buffer_ptr, reinterpret_cast<char*>(&res) + (address & alignment), todo);

        raw_buffer_ptr += todo;
        address += todo;
        length -= todo;
    }

    // NOLINTEND(google-runtime-int)

    return result_buffer;
}

Result<void> PtraceMemoryIO::write_block_impl(std::uintptr_t address, const ByteVector& data) {
    // TODO: Detetermine whether alignment logic is necessary
    // static constexpr int ALIGNMENT = sizeof(long); // NOLINT(google-runtime-int) - based on ptrace(2) spec
    // ASSERT(/*addr % ALIGNMENT == 0 &&*/ data.size() % ALIGNMENT == 0,
    //        "'data.size()' and 'n' must be multiples of alignement - sizeof(long)");

    // NOLINTBEGIN(google-runtime-int) - based on ptrace(2) spec

    auto length = data.size();
    auto iter = data.begin();

    long ptrace_write_data{};

    while (length != 0) {
        size_t todo = std::min(length, sizeof(long));

        // move the bytes at the back of `ptrace_write_data` that we need to write AGAIN
        // to the front of the data buffer
        if (todo < 8) {
            long temp_buffer{};
            std::memcpy(reinterpret_cast<char*>(&temp_buffer), reinterpret_cast<const char*>(&ptrace_write_data) + todo,
                        sizeof(long) - todo);
            std::memcpy(reinterpret_cast<char*>(&ptrace_write_data), reinterpret_cast<const char*>(&temp_buffer),
                        sizeof(long) - todo);
        }
        std::memcpy(reinterpret_cast<char*>(&ptrace_write_data) + sizeof(long) - todo,
                    reinterpret_cast<const char*>(&*iter), todo);

        std::size_t ptrace_write_addr = address - 8 + todo;
        TRYE(linux::ptrace(PTRACE_POKETEXT, get_pid(), ptrace_write_addr, ptrace_write_data),
             ErrorKind::SyscallFailure);

        iter += static_cast<std::ptrdiff_t>(todo);
        address += todo;
        length -= todo;
    }

    // NOLINTEND(google-runtime-int)

    return {};
}

} // namespace asmgrader
