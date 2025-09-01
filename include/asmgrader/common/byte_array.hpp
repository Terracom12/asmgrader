#pragma once

#include <asmgrader/common/byte.hpp>
#include <asmgrader/common/os.hpp>
#include <asmgrader/meta/static_size.hpp>

#include <gsl/assert>
#include <libassert/assert.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/iterator/traits.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/range/traits.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace asmgrader {

template <std::size_t Size, EndiannessKind Endianness>
class ByteArray
{
public:
    // A bunch of aliases for use with algorithm templates that check for them
    using value_type = Byte;
    using allocator_type = std::allocator<Byte>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = std::array<Byte, Size>::iterator;
    using const_iterator = std::array<Byte, Size>::const_iterator;
    using reverse_iterator = std::array<Byte, Size>::reverse_iterator;
    using const_reverse_iterator = std::array<Byte, Size>::const_reverse_iterator;

    Byte& operator[](size_t idx) { return data[idx]; }

    const Byte& operator[](size_t idx) const { return data[idx]; }

    constexpr Byte& at(size_t idx) { return data.at(idx); }

    const Byte& at(size_t idx) const { return data.at(idx); }

    constexpr bool empty() const { return data.empty(); }

    constexpr auto begin() { return data.begin(); }

    constexpr auto begin() const { return data.begin(); }

    constexpr auto cbegin() const { return data.cend(); }

    constexpr auto end() { return data.end(); }

    constexpr auto end() const { return data.end(); }

    constexpr auto cend() const { return data.cend(); }

    constexpr std::size_t size() const { return data.size(); }

    std::array<Byte, Size> data;

private:
    template <ranges::range Range>
        requires requires(ranges::range_value_t<Range> value) {
            { Byte{value} };
        }
    constexpr void from_range(Range&& range) {
        ASSERT(get_static_size_or<Range>(ranges::size(range)) <= Size);

        ranges::transform(std::forward<Range>(range), data.begin(), [](Byte value) { return Byte{value}; });
    }
};

/// Deduction guide
template <typename T, typename... U>
ByteArray(T, U...) -> ByteArray<sizeof...(U) + 1, EndiannessKind::Native>;

template <std::size_t N>
using NativeByteArray = ByteArray<N, EndiannessKind::Native>;

static_assert(std::is_aggregate_v<NativeByteArray<0>>);
static_assert(std::is_aggregate_v<NativeByteArray<1>>);
static_assert(std::is_aggregate_v<NativeByteArray<10>>);

} // namespace asmgrader

namespace std {

/// Specialization of tuple_size to play nice with algorithms that work on tuple-like types
template <std::size_t Size>
// See: https://en.cppreference.com/w/cpp/utility/tuple_size.html
// NOLINTNEXTLINE(cert-dcl58-cpp) - this is well defined and probably a clang-tidy bug
struct tuple_size<::asmgrader::NativeByteArray<Size>> : public std::integral_constant<std::size_t, Size>
{
};

/// Specialization of tuple_element to play nice with algorithms that work on tuple-like types
template <std::size_t I, std::size_t N>
    requires(I < N)
// See: https://en.cppreference.com/w/cpp/container/array/tuple_element.html
// NOLINTNEXTLINE(cert-dcl58-cpp) - this is well defined and probably a clang-tidy bug
struct tuple_element<I, ::asmgrader::NativeByteArray<N>>
{
    using type = asmgrader::Byte;
};

} // namespace std

/// Specialization of get to play nice with algorithms that work on tuple-like types
template <std::size_t I, std::size_t N>
    requires(I < N)
constexpr ::asmgrader::Byte& get(::asmgrader::NativeByteArray<N>& arr) {
    return arr.data.at(I);
}

/// \overload
template <std::size_t I, std::size_t N>
    requires(I < N)
constexpr const ::asmgrader::Byte& get(const ::asmgrader::NativeByteArray<N>& arr) {
    return arr.data.at(I);
}

/// \overload
template <std::size_t I, std::size_t N>
    requires(I < N)
// TODO: double check this rvalue usage
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) - I think this is correct?
constexpr ::asmgrader::Byte&& get(::asmgrader::NativeByteArray<N>&& arr) {
    return arr.data.at(I);
}

/// \overload
template <std::size_t I, std::size_t N>
    requires(I < N)
constexpr const ::asmgrader::Byte&& get(const ::asmgrader::NativeByteArray<N>&& arr) {
    return arr.data.at(I);
}
