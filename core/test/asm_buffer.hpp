#pragma once

#include "asm_data.hpp"
#include "program/program.hpp"
#include "util/byte_array.hpp"
#include "util/error_types.hpp"

#include <range/v3/algorithm/fill.hpp>
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

    std::size_t size() const;

    util::Result<std::string> str() const;

    util::Result<ByteArray<NumBytes>> fill(std::byte byte) const;

private:
    static std::uintptr_t get_alloced_address(Program& prog) { return prog.alloc_mem(NumBytes); }
};
template <std::size_t NumBytes>
std::size_t AsmBuffer<NumBytes>::size() const {
    return NumBytes;
}

template <std::size_t NumBytes>
util::Result<ByteArray<NumBytes>> AsmBuffer<NumBytes>::fill(std::byte byte) const {
    auto prev = TRY(AsmData<ByteArray<NumBytes>>::get_value());

    ByteArray<NumBytes> buf;
    ranges::fill(buf, byte);

    TRY(AsmData<ByteArray<NumBytes>>::set_value(buf));

    return prev;
}

template <std::size_t NumBytes>
util::Result<std::string> AsmBuffer<NumBytes>::str() const {
    MemoryIOBase& mio = AsmData<ByteArray<NumBytes>>::get_program().get_subproc().get_tracer().get_memory_io();

    auto addr = AsmData<ByteArray<NumBytes>>::get_address();

    auto byte_str = TRY(mio.read<ByteArray<NumBytes>>(addr));

    return byte_str | ranges::views::transform([](std::byte byte) { return std::to_integer<char>(byte); }) |
           ranges::views::take_while([](char chr) { return chr != 0; }) | ranges::to<std::string>();
}

template <std::size_t NumBytes>
AsmBuffer<NumBytes>::AsmBuffer(Program& prog)
    : AsmData<ByteArray<NumBytes>>{prog, get_alloced_address(prog)} {}
