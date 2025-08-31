#pragma once

#include <asmgrader/api/asm_data.hpp>
#include <asmgrader/common/byte.hpp>
#include <asmgrader/common/byte_array.hpp>
#include <asmgrader/common/macros.hpp>
#include <asmgrader/program/program.hpp>
#include <asmgrader/subprocess/memory/memory_io_base.hpp>

#include <range/v3/algorithm/fill.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace asmgrader {

template <std::size_t NumBytes>
class AsmBuffer : public AsmData<ByteArray<NumBytes>>
{
public:
    explicit AsmBuffer(Program& prog);

    std::size_t size() const;

    std::string str() const;

    ByteArray<NumBytes> fill(Byte byte) const;

private:
    static std::uintptr_t get_alloced_address(Program& prog) { return prog.alloc_mem(NumBytes); }
};

template <std::size_t NumBytes>
std::size_t AsmBuffer<NumBytes>::size() const {
    return NumBytes;
}

template <std::size_t NumBytes>
ByteArray<NumBytes> AsmBuffer<NumBytes>::fill(Byte byte) const {
    auto prev = TRY_OR_THROW(AsmData<ByteArray<NumBytes>>::get_value(), "could not read previous data value");

    ByteArray<NumBytes> buf;
    ranges::fill(buf, byte);

    AsmData<ByteArray<NumBytes>>::set_value(buf);

    return prev;
}

template <std::size_t NumBytes>
std::string AsmBuffer<NumBytes>::str() const {
    MemoryIOBase& mio = AsmData<ByteArray<NumBytes>>::get_program().get_subproc().get_tracer().get_memory_io();

    auto addr = AsmData<ByteArray<NumBytes>>::get_address();

    auto byte_str = TRY_OR_THROW(mio.read<ByteArray<NumBytes>>(addr), "failed to read buffer");

    return byte_str | ranges::views::transform([](Byte byte) { return static_cast<char>(byte.value); }) |
           ranges::views::take_while([](char chr) { return chr != 0; }) | ranges::to<std::string>();
}

template <std::size_t NumBytes>
AsmBuffer<NumBytes>::AsmBuffer(Program& prog)
    : AsmData<ByteArray<NumBytes>>{prog, get_alloced_address(prog)} {}

} // namespace asmgrader
