#pragma once

#include <cstddef>
#include <type_traits>

namespace asmgrader {

// TODO: Use boost::mp11::mp_count_if instead

template <template <typename> typename Cond, typename... Ts>
struct count_if
{
    static constexpr std::size_t value = (Cond<Ts>::value + ... + 0);
};

template <template <typename> typename Cond, typename... Ts>
constexpr std::size_t count_if_v = count_if<Cond, Ts...>::value;

template <template <typename> typename Cond, typename... Ts>
struct count_if_not
{
    static constexpr std::size_t value = sizeof...(Ts) - (Cond<Ts>::value + ... + 0);
};

template <template <typename> typename Cond, typename... Ts>
constexpr std::size_t count_if_not_v = count_if_not<Cond, Ts...>::value;

static_assert(count_if_v<std::is_floating_point, float, int, double> == 2);
static_assert(count_if_v<std::is_integral, float, int, double> == 1);
static_assert(count_if_v<std::is_integral> == 0);

} // namespace asmgrader
