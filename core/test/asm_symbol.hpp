#pragma once

#include "meta/always_false.hpp"
#include "program/program.hpp"
#include "subprocess/memory/concepts.hpp"
#include "util/error_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

template <typename T>
    requires(MemoryReadSupportedType<T>)
class AsmSymbol
{
public:
    AsmSymbol(Program& prog, std::string name, std::uintptr_t address);
    AsmSymbol(Program& prog, std::string name, util::ErrorKind resolution_err);

    std::string_view get_name() const;

    util::Result<T> get_value() const;

private:
    Program* prog_;

    std::uintptr_t address_;
    std::string name_;
    std::optional<util::ErrorKind> resolution_err_;
};

template <typename T>
    requires(MemoryReadSupportedType<T>)
std::string_view AsmSymbol<T>::get_name() const {
    return name_;
}

template <typename T>
    requires(MemoryReadSupportedType<T>)
util::Result<T> AsmSymbol<T>::get_value() const {
    if (resolution_err_.has_value()) {
        return *resolution_err_;
    }

    MemoryIOBase& mio = prog_->get_subproc().get_tracer().get_memory_io();

    T value = TRY(mio.read<T>(address_));

    std::string val_str = "<unformattable>";
    if constexpr (fmt::formattable<T>) {
        val_str = fmt::format("{}", value);
    }
    LOG_DEBUG("Read value {} for symbol {:?} @ 0x{:X}", val_str, name_, address_);

    return value;
}

template <typename T>
    requires(MemoryReadSupportedType<T>)
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, std::uintptr_t address)
    : prog_{&prog}
    , address_{address}
    , name_{std::move(name)} {}

template <typename T>
    requires(MemoryReadSupportedType<T>)
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, util::ErrorKind resolution_err)
    : prog_{&prog}
    , address_{0x0}
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}
