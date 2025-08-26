#include "subprocess/memory/memory_io_base.hpp"

#include "common/byte_vector.hpp"
#include "common/error_types.hpp"

#include <libassert/assert.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace asmgrader {

Result<ByteVector> MemoryIOBase::read_until(std::uintptr_t address, const std::function<bool(std::byte)>& predicate) {
    ByteVector result;

    bool pred_satisfied = false;
    for (std::uintptr_t current_address = address; !pred_satisfied; current_address += 8) {
        auto next_block = TRY(read_block_impl(current_address, 8));

        for (std::byte byte : next_block) {
            if (predicate(byte)) {
                pred_satisfied = true;
                break;
            }

            result.push_back(byte);
        }
    }

    return result;
}

Result<ByteVector> MemoryIOBase::read_until(std::uintptr_t address,
                                            const std::function<bool(std::span<const std::byte>)>& predicate,
                                            std::size_t block_size) {
    ASSERT(block_size > 0);

    ByteVector result;

    for (std::uintptr_t current_address = address;; current_address += block_size) {
        ByteVector block = TRY(read_block_impl(current_address, block_size));

        if (!predicate(block)) {
            break;
        }

        result.insert(result.end(), block.begin(), block.end());
    }

    return result;
}

} // namespace asmgrader
