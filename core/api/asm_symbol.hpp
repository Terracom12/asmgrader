#pragma once

#include "exceptions.hpp"
#include "fmt/base.h"
#include "logging.hpp"
#include "program/program.hpp"
#include "api/asm_data.hpp"
#include "util/error_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

template <typename T>
class AsmSymbol : AsmData<T>
{
public:
    AsmSymbol(Program& prog, std::string name, std::uintptr_t address);
    AsmSymbol(Program& prog, std::string name, util::ErrorKind resolution_err);

    std::string_view get_name() const;

    T get_value() const override;

private:
    std::string name_;
    std::optional<util::ErrorKind> resolution_err_;
};

template <typename T>
std::string_view AsmSymbol<T>::get_name() const {
    return name_;
}

template <typename T>
T AsmSymbol<T>::get_value() const {
    if (resolution_err_.has_value()) {
        throw ContextInternalError(*resolution_err_, "failed to resolve function");
    }

    auto value = AsmData<T>::get_value_impl();

    std::string val_str = "<unformattable>";
    if constexpr (fmt::formattable<T>) {
        val_str = fmt::format("{}", value);
    }
    LOG_DEBUG("Read value {} for symbol {:?} @ 0x{:X}", val_str, name_, AsmData<T>::get_address());

    return value;
}

template <typename T>
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, std::uintptr_t address)
    : AsmData<T>(prog, address)
    , name_{std::move(name)} {}

template <typename T>
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, util::ErrorKind resolution_err)
    : AsmData<T>(prog, 0)
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}
