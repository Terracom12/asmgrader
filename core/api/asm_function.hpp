#pragma once

#include "common/error_types.hpp"
#include "meta/always_false.hpp"
#include "program/program.hpp"
#include "subprocess/memory/concepts.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

template <typename T>
class AsmFunction
{
    static_assert(util::always_false_v<T>, "AsmFunction is not specialized for non-function types!");
};

template <typename Ret, typename... Args>
class AsmFunction<Ret(Args...)>
{
public:
    AsmFunction(Program& prog, std::string name, std::uintptr_t address);
    AsmFunction(Program& prog, std::string name, util::ErrorKind resolution_err);

    template <MemoryIOCompatible<Args>... Ts>
        requires(sizeof...(Ts) == sizeof...(Args))
    util::Result<Ret> operator()(Ts&&... args) {
        if (resolution_err_.has_value()) {
            return *resolution_err_;
        }

        (check_arg<Ts>(), ...);

        return prog_->call_function<Ret(Args...)>(address_, std::forward<Ts>(args)...);
    }

private:
    template <typename T>
    void check_arg() {
        using NormT = std::remove_cvref_t<T>;
        static_assert(!std::is_pointer_v<NormT> && !std::is_array_v<NormT>,
                      "Passing a raw pointer as argument for an asm function, which is probably not what you meant to "
                      "do. See docs on program memory for more info.");
    }

    Program* prog_;

    std::uintptr_t address_;
    std::string name_;
    std::optional<util::ErrorKind> resolution_err_;
};

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, std::uintptr_t address)
    : prog_{&prog}
    , address_{address}
    , name_{std::move(name)} {}

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, util::ErrorKind resolution_err)
    : prog_{&prog}
    , address_{0x0}
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}
