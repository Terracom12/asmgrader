#pragma once

#include <asmgrader/common/error_types.hpp>
#include <asmgrader/meta/always_false.hpp>
#include <asmgrader/program/program.hpp>
#include <asmgrader/subprocess/memory/concepts.hpp>

#include <fmt/base.h>

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace asmgrader {

/// Transparent wrapper around Result<Ret>, as far as the user is concerned.
///
/// Internally provides serialized_args and function_name member variables for
/// use in requirement serialization.
template <typename Ret, typename... Args>
class AsmFunctionResult : public Result<Ret>
{
public:
    // NOLINTNEXTLINE(readability-identifier-naming) - rational: don't shadow member variables
    constexpr AsmFunctionResult(std::tuple<std::decay_t<Args>...>&& args_, std::string_view function_name_)
        : args{std::move(args_)}
        , function_name{function_name_} {}

    template <typename U>
    void set_result(U&& val) {
        Result<Ret>::operator=(std::forward<U>(val));
    }

    // not sure if these are really needed, but I'm including them to be safe
    using Result<Ret>::operator==;
    using Result<Ret>::operator<=>;

    std::tuple<std::decay_t<Args>...> args;
    std::string_view function_name;
};

template <typename T>
class AsmFunction
{
    static_assert(always_false_v<T>, "AsmFunction is not specialized for non-function types!");
};

template <typename Ret, typename... Args>
class AsmFunction<Ret(Args...)>
{
public:
    AsmFunction(Program& prog, std::string name, std::uintptr_t address);
    AsmFunction(Program& prog, std::string name, ErrorKind resolution_err);

    template <MemoryIOCompatible<Args>... Ts>
        requires(sizeof...(Ts) == sizeof...(Args))
    AsmFunctionResult<Ret, Ts...> operator()(Ts&&... args) {
        static_assert((true && ... && std::copyable<std::decay_t<Ts>>), "All arguments must be copyable");
        (check_arg<Ts>(), ...);

        // making copies of args...
        AsmFunctionResult<Ret, Ts...> res{{args...}, name_};

        if (resolution_err_.has_value()) {
            res.set_result(*resolution_err_);
            return res;
        }

        res.set_result(prog_->call_function<Ret(Args...)>(address_, std::forward<Ts>(args)...));

        return res;
    }

    const std::string& get_name() const { return name_; }

private:
    template <typename T>
    void check_arg() {
        using NormT = std::decay_t<T>;
        static_assert(!std::is_pointer_v<NormT> && !std::is_array_v<NormT>,
                      "Passing a raw pointer as argument for an asm function, which is probably not what you meant to "
                      "do. See docs on program memory for more info.");
    }

    Program* prog_;

    std::uintptr_t address_;
    std::string name_;
    std::optional<ErrorKind> resolution_err_;
};

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, std::uintptr_t address)
    : prog_{&prog}
    , address_{address}
    , name_{std::move(name)} {}

template <typename Ret, typename... Args>
AsmFunction<Ret(Args...)>::AsmFunction(Program& prog, std::string name, ErrorKind resolution_err)
    : prog_{&prog}
    , address_{0x0}
    , name_{std::move(name)}
    , resolution_err_{resolution_err} {}

} // namespace asmgrader

template <typename Ret, typename... Ts>
struct fmt::formatter<::asmgrader::AsmFunctionResult<Ret, Ts...>> : formatter<::asmgrader::Result<Ret>>
{
};
