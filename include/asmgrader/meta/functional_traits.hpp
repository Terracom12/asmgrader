#pragma once

#include <asmgrader/meta/always_false.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_list.hpp>

#include <cstddef>

namespace asmgrader {

template <typename Func>
struct FunctionTraits
{
    static_assert(always_false_v<Func>, "Must be instantiated with a callable type");
};

template <typename FuncRet, typename... FuncArgs>
struct FunctionTraits<FuncRet(FuncArgs...)>
{
    using Ret = FuncRet;

    using Args = boost::mp11::mp_list<FuncArgs...>;

    template <std::size_t I>
        requires(I < boost::mp11::mp_size<Args>::value)
    struct Arg
    {
        using type = boost::mp11::mp_at_c<Args, I>;
    };
};

} // namespace asmgrader
