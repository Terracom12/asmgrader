#pragma once

#include <cstddef>
#include <tuple>

namespace util {

template <typename Func>
struct FunctionType
{
    static_assert(sizeof(Func) && false, "Must be instantiated with a callable type");
};

template <typename Ret, typename... Args>
struct FunctionType<Ret(Args...)>
{
    using ret = Ret;

    static constexpr auto num_args = sizeof...(Args);

    using args = std::tuple<Args...>;

    template <std::size_t I>
        requires(I < num_args)
    struct arg
    {
        using type = std::tuple_element_t<I, std::tuple<Args...>>;
    };
};

} // namespace util
