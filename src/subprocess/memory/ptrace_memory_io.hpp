#pragma once

#include <asmgrader/common/byte_vector.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/subprocess/memory/memory_io_base.hpp>

#include <cstddef>
#include <cstdint>

namespace asmgrader {

/// TraceeMemory implemented using ptrace(2) commands
class PtraceMemoryIO final : public MemoryIOBase
{
    using MemoryIOBase::MemoryIOBase;

private:
    Result<NativeByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override;
    Result<void> write_block_impl(std::uintptr_t address, const NativeByteVector& data) override;
};

} // namespace asmgrader
