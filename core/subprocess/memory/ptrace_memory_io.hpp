#pragma once

#include "common/byte_vector.hpp"
#include "subprocess/memory/memory_io.hpp"

#include <cstddef>

namespace asmgrader {

/// TraceeMemory implemented using ptrace(2) commands
class PtraceMemoryIO final : public MemoryIOBase
{
    using MemoryIOBase::MemoryIOBase;

private:
    Result<ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override;
    // ByteVector read_until_impl(std::uintptr_t address, const std::function<bool(std::byte)>& predicate) override;
    Result<void> write_block_impl(std::uintptr_t address, const ByteVector& data) override;
};

} // namespace asmgrader
