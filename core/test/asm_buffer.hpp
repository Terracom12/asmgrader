#pragma once

#include "asm_data.hpp"
#include "program/program.hpp"
#include "util/byte_array.hpp"
#include "util/error_types.hpp"

#include <range/v3/to_container.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

template <std::size_t NumBytes>
class AsmBuffer : public AsmData<ByteArray<NumBytes>>
{
public:
    explicit AsmBuffer(Program& prog);

    virtual ~AsmBuffer() = default;

    util::Result<std::string> str() const;

private:
    std::uintptr_t get_alloced_address(Program& prog) const { return prog.alloc_mem(NumBytes); }
};

template <std::size_t NumBytes>
util::Result<std::string> AsmBuffer<NumBytes>::str() const {
    MemoryIOBase& mio = AsmData<ByteArray<NumBytes>>::get_program().get_subproc().get_tracer().get_memory_io();

    auto addr = AsmData<ByteArray<NumBytes>>::get_address();

    auto byte_str = TRY(mio.read<ByteArray<NumBytes>>(addr));

    return byte_str | ranges::views::transform([](std::byte byte) { return static_cast<char>(byte); }) |
           ranges::views::take_while([](char chr) { return chr != 0; }) | ranges::to<std::string>();
}

template <std::size_t NumBytes>
AsmBuffer<NumBytes>::AsmBuffer(Program& prog)
    : AsmData<ByteArray<NumBytes>>{prog, get_alloced_address(prog)} {}
