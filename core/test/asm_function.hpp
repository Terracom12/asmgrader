#pragma once

#include "meta/always_false.hpp"
#include "util/error_types.hpp"

#include <cstdint>
#include <string>

// class TestBase;

template <typename T>
class AsmFunction
{
    static_assert(util::always_false_v<T>, "AsmFunction is not specialized for non-function types!");
};

template <typename Ret, typename... Args>
class AsmFunction<Ret(Args...)>
{
public:
    AsmFunction(std::string name, std::uintptr_t address);

    util::Result<Ret> operator()(Args... args) { return {}; }

private:
    std::uintptr_t address_;
    std::string name_;
};

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(std::string name, std::uintptr_t address)
    : address_{address}
    , name_{std::move(name)} {}
