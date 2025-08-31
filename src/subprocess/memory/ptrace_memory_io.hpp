#pragma once

#include "common/byte_vector.hpp"
#include "common/error_types.hpp"
#include "subprocess/memory/memory_io_base.hpp"

#include <cstddef>
#include <cstdint>

namespace asmgrader {

/// TraceeMemory implemented using ptrace(2) commands
class PtraceMemoryIO final : public MemoryIOBase
{
    using MemoryIOBase::MemoryIOBase;

private:
    Result<ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override;
    Result<void> write_block_impl(std::uintptr_t address, const ByteVector& data) override;
};

} // namespace asmgrader
