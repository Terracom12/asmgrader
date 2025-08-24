#pragma once

#include "meta/always_false.hpp"
#include "subprocess/memory/concepts.hpp"

namespace asmgrader {

namespace detail {

template <typename Func, typename... Args>
struct CompatibleFunctionArgsImpl
{
    static_assert(always_false_v<Func>, "Not supported for non-function types");
};

template <typename Ret, typename... FuncArgs, typename... Args>
struct CompatibleFunctionArgsImpl<Ret(FuncArgs...), Args...>
{
    static constexpr bool value = (MemoryIOCompatible<FuncArgs, Args> && ...);
};

} // namespace detail

template <typename Func, typename... Args>
concept CompatibleFunctionArgs = detail::CompatibleFunctionArgsImpl<Func, Args...>::value;

} // namespace asmgrader
