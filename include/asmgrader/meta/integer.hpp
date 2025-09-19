#pragma once

#include <asmgrader/common/aliases.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/integral.hpp>

#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace asmgrader {

namespace detail {
namespace mp = boost::mp11;

using sized_uint_map = mp::mp_list<u8, u16, u32, u64>;
using sized_int_map = mp::mp_list<i8, i16, i32, i64>;

template <int I>
using sized_int_impl = mp::mp_at<sized_int_map, mp::mp_int<I>>;

template <int I>
using sized_uint_impl = mp::mp_at<sized_uint_map, mp::mp_int<I>>;
} // namespace detail

template <std::size_t NumBits>
using sized_int_t = detail::sized_int_impl<std::bit_width(NumBits) - 1>;

template <std::size_t NumBits>
using sized_uint_t = detail::sized_uint_impl<std::bit_width(NumBits) - 1>;

/// This was a little confusing (to me at least)
///   `digits10` => maximum number of base-10 digits that can be represented with IntType
/// below is a definition for the *inverse* of that
///   i.e. => maximmum number of base-10 digits that IntType can represent
/// For both signed and unsigned integer types, the value that produces the maximal amount
/// of base10 digits will never itself be a power of 10. The proof for this is trivial
/// using parity to check when all bits are set [2^(n+1) - 1] and factorization to check
/// 2^n (for the negative complement of signed min).
///
/// Using the property above, we know that the maximum maximum number of base-10 digits
/// that IntType can represent is `digits10` + 1
template <typename IntType>
    requires std::integral<std::decay_t<IntType>>
constexpr std::size_t digits10_max_count = std::numeric_limits<std::decay_t<IntType>>::digits10 + 1;

static_assert(digits10_max_count<i8> == 3);
static_assert(digits10_max_count<u8> == 3);
static_assert(digits10_max_count<i16> == 5);
static_assert(digits10_max_count<u16> == 5);
static_assert(digits10_max_count<i32> == 10);
static_assert(digits10_max_count<u32> == 10);
static_assert(digits10_max_count<i64> == 19);
static_assert(digits10_max_count<u64> == 20);

} // namespace asmgrader
