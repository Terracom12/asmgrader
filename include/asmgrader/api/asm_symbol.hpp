#pragma once

#include <asmgrader/api/asm_data.hpp>
#include <asmgrader/api/expression_inspection.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/exceptions.hpp>
#include <asmgrader/logging.hpp>
#include <asmgrader/program/program.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace asmgrader {

/// Transparent wrapper around Result<T>, as far as the user is concerned.
///
/// Internally provides serialized_args and function_name member variables for
/// use in requirement serialization.
template <typename T>
class AsmSymbolResult : public Result<T>
{
public:
    // NOLINTNEXTLINE(readability-identifier-naming) - rational: don't shadow member variables
    explicit constexpr AsmSymbolResult(std::string_view symbol_name_)
        : symbol_name{symbol_name_} {}

    template <typename U>
    void set_result(U&& val) {
        Result<T>::operator=(std::forward<U>(val));
    }

    // not sure if these are really needed, but I'm including them to be safe
    using Result<T>::operator==;
    using Result<T>::operator<=>;

    std::string_view symbol_name;

    std::string repr(std::span<const inspection::Token> /*tokens*/, std::string_view /*raw_str*/) const {
        return fmt::format("*{}", symbol_name);
    }

    // using the default `str` implementation for the base Expected type
};

template <typename T>
class AsmSymbol : public AsmData<T>
{
public:
    AsmSymbol(Program& prog, std::string name, std::uintptr_t address);
    AsmSymbol(Program& prog, std::string name, ErrorKind resolution_err);

    std::string_view get_name() const;

    AsmSymbolResult<T> get_value() const;

private:
    std::string name_;
    std::optional<ErrorKind> resolution_err_;
};

template <typename T>
std::string_view AsmSymbol<T>::get_name() const {
    return name_;
}

template <typename T>
AsmSymbolResult<T> AsmSymbol<T>::get_value() const {
    AsmSymbolResult<T> res{name_};

    if (resolution_err_.has_value()) {
        res.set_result(*resolution_err_);
        return res;
    }

    auto value = AsmData<T>::get_value_impl();

    std::string val_str = "<unformattable>";
    if constexpr (fmt::formattable<T>) {
        val_str = fmt::format("{}", value);
    }
    LOG_DEBUG("Read value {} for symbol {:?} @ 0x{:X}", val_str, name_, AsmData<T>::get_address());

    res.set_result(value);

    return res;
}

template <typename T>
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, std::uintptr_t address)
    : AsmData<T>(prog, address)
    , name_{std::move(name)} {}

template <typename T>
AsmSymbol<T>::AsmSymbol(Program& prog, std::string name, ErrorKind resolution_err)
    : AsmData<T>(prog, 0)
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}

} // namespace asmgrader
