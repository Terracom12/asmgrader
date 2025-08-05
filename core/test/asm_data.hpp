#pragma once

#include "logging.hpp"
#include "program/program.hpp"
#include "subprocess/memory/concepts.hpp"
#include "util/error_types.hpp"

#include <concepts>
#include <cstdint>
#include <string>
#include <utility>

template <typename T>
    requires(MemoryReadSupported<T>)
class AsmData
{
public:
    AsmData(Program& prog, std::uintptr_t address);

    std::uintptr_t get_address() const { return address_; }

    /// Get the value currently present in the asm program
    virtual util::Result<T> get_value() const;

    /// Set the value of type ``T`` in the asm program
    ///
    /// Returns the previously held value
    template <MemoryIOCompatible<T> U>
    // this constraint permits a simplified writing interface
    // e.g., a std::string can be written to a char* buffer
        requires(MemoryWriteSupported<U>)
    util::Result<T> set_value(const U& val) const;

    /// Zeros the object of type ``T`` residing in the asm program
    ///
    /// This is equivalent to ``set_value(T{})`` if ``T`` is
    /// default-constructable. If not, simply sets the bytes
    /// in the range [address, address + sizeof(T)] to 0
    util::Result<T> zero() const
        requires(MemoryWriteSupported<T>);

protected:
    util::Result<T> get_value_impl() const;

    Program& get_program() const { return *prog_; }

private:
    Program* prog_;

    std::uintptr_t address_;
};
template <typename T>
    requires(MemoryReadSupported<T>)
template <MemoryIOCompatible<T> U>
    requires(MemoryWriteSupported<U>)
util::Result<T> AsmData<T>::set_value(const U& val) const {
    auto prev = TRY(get_value());

    MemoryIOBase& mio = prog_->get_subproc().get_tracer().get_memory_io();
    TRY(mio.write(address_, val));

    return prev;
}

template <typename T>
    requires(MemoryReadSupported<T>)
util::Result<T> AsmData<T>::zero() const
    requires(MemoryWriteSupported<T>)
{
    if constexpr (std::default_initializable<T>) {
        return set_value(T{});
    } else {
        UNIMPLEMENTED("zero-initialization is not implemented for types that are not default initializable!");
    }
}

template <typename T>
    requires(MemoryReadSupported<T>)
util::Result<T> AsmData<T>::get_value_impl() const {
    MemoryIOBase& mio = prog_->get_subproc().get_tracer().get_memory_io();

    return mio.read<T>(address_);
}

template <typename T>
    requires(MemoryReadSupported<T>)
util::Result<T> AsmData<T>::get_value() const {
    auto value = TRY(get_value_impl());

    std::string val_str = "<unformattable>";
    if constexpr (fmt::formattable<T>) {
        val_str = fmt::format("{}", value);
    }
    LOG_DEBUG("Read value {} for @ 0x{:X}", val_str, address_);

    return value;
}

template <typename T>
    requires(MemoryReadSupported<T>)
AsmData<T>::AsmData(Program& prog, std::uintptr_t address)
    : prog_{&prog}
    , address_{address} {}
