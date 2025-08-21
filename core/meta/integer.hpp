#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/integral.hpp>

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace meta {

namespace detail {
namespace mp = boost::mp11;

using sized_uint_map = mp::mp_list<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;
using sized_int_map = mp::mp_list<std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

template <std::size_t I>
using sized_int_impl = mp::mp_at<sized_int_map, mp::mp_int<I>>;

template <std::size_t I>
using sized_uint_impl = mp::mp_at<sized_uint_map, mp::mp_int<I>>;
} // namespace detail

template <std::size_t NumBits>
using sized_int_t = detail::sized_int_impl<std::bit_width(NumBits) - 1>;

template <std::size_t NumBits>
using sized_uint_t = detail::sized_uint_impl<std::bit_width(NumBits) - 1>;

} // namespace meta
