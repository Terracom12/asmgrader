#pragma once

#include "subprocess/memory/memory_io.hpp"
#include "util/byte_vector.hpp"

#include <cstddef>

/// TraceeMemory implemented using ptrace(2) commands
class PtraceMemoryIO final : public MemoryIOBase
{
    using MemoryIOBase::MemoryIOBase;

private:
    util::Result<ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override;
    // ByteVector read_until_impl(std::uintptr_t address, const std::function<bool(std::byte)>& predicate) override;
    util::Result<void> write_block_impl(std::uintptr_t address, const ByteVector& data) override;
};
