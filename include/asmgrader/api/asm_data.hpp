#pragma once

#include <asmgrader/common/error_types.hpp>
#include <asmgrader/common/macros.hpp>
#include <asmgrader/logging.hpp>
#include <asmgrader/program/program.hpp>
#include <asmgrader/subprocess/memory/concepts.hpp>

#include <fmt/base.h>

#include <concepts>
#include <cstdint>
#include <string>

namespace asmgrader {

template <typename T>
    requires(MemoryReadSupported<T>)
class AsmData
{
public:
    AsmData(Program& prog, std::uintptr_t address);

    virtual ~AsmData() = default;

    // NOLINTNEXTLINE(google-runtime-operator) - nicer semantics?
    std::uintptr_t operator&() const { return address_; }

    Result<T> operator*() const { return get_value(); }

    std::uintptr_t get_address() const { return address_; }

    /// Get the value currently present in the asm program
    Result<T> get_value() const;

    /// Set the value of type ``T`` in the asm program
    ///
    /// Returns the previously held value
    template <MemoryIOCompatible<T> U>
    // this constraint permits a simplified writing interface
    // e.g., a std::string can be written to a char* buffer
        requires(MemoryWriteSupported<U>)
    T set_value(const U& val) const;

    /// Zeros the object of type ``T`` residing in the asm program
    ///
    /// This is equivalent to ``set_value(T{})`` if ``T`` is
    /// default-constructable. If not, simply sets the bytes
    /// in the range [address, address + sizeof(T)] to 0
    T zero() const
        requires(MemoryWriteSupported<T>);

protected:
    Result<T> get_value_impl() const;

    Program& get_program() const { return *prog_; }

private:
    Program* prog_;

    std::uintptr_t address_;
};

template <typename T>
    requires(MemoryReadSupported<T>)
template <MemoryIOCompatible<T> U>
    requires(MemoryWriteSupported<U>)
T AsmData<T>::set_value(const U& val) const {
    auto prev = TRY_OR_THROW(get_value(), "could not read previous data value");

    MemoryIOBase& mio = prog_->get_subproc().get_tracer().get_memory_io();
    TRY_OR_THROW(mio.write(address_, val), "could not set data value");

    return prev;
}

template <typename T>
    requires(MemoryReadSupported<T>)
T AsmData<T>::zero() const
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
Result<T> AsmData<T>::get_value_impl() const {
    MemoryIOBase& mio = prog_->get_subproc().get_tracer().get_memory_io();

    return mio.read<T>(address_);
}

template <typename T>
    requires(MemoryReadSupported<T>)
Result<T> AsmData<T>::get_value() const {
    auto value = get_value_impl();

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

} // namespace asmgrader
