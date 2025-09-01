#pragma once

#include <asmgrader/common/byte.hpp>
#include <asmgrader/common/byte_array.hpp>
#include <asmgrader/common/byte_vector.hpp>
#include <asmgrader/common/os.hpp>
#include <asmgrader/common/to_static_range.hpp>
#include <asmgrader/meta/concept_wrapper.hpp>
#include <asmgrader/meta/static_size.hpp>

#include <range/v3/algorithm/copy_n.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/reverse.hpp>
#include <range/v3/iterator/access.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/iterator/traits.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/range/traits.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/semiregular_box.hpp>
#include <range/v3/view/adaptor.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/facade.hpp>
#include <range/v3/view/for_each.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/single.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/view.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace asmgrader {

namespace detail {

template <EndiannessKind To, ranges::range Range>
constexpr void maybe_reverse_endianness(Range& range) {
    if constexpr (To != EndiannessKind::Native) {
        ranges::reverse(range);
    }
}

template <template <std::size_t, EndiannessKind> typename SizedTo, std::size_t Size>
struct partial_bytearray_spec
{
    template <EndiannessKind Endianness>
    using templated = SizedTo<Size, Endianness>;
};

} // namespace detail

/**
 * \brief Reinterpret a range of trivially copyable values as a ByteVector of specified endianness,
 * or as a ByteArray of specified or deduced size and specified endianness.
 *
 * NativeByteVector or NativeByteArray aliases may alternatively provided, ommitting the endianness parameter.
 *
 *
 * \tparam To a ByteVector class template
 * \tparam Endianness the endianness of the ByteVector
 * \tparam Range the type of the range
 * \param range a range of trivially copyable values
 */
template <template <EndiannessKind> typename To, EndiannessKind Endianness, ranges::range Range>
    requires(std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    To<Endianness> buffer{};

    if constexpr (requires(To<Endianness> rng, std::size_t new_size) { rng.resize(new_size); }) {
        auto size = sizeof(ranges::range_value_t<Range>) * range.size();
        buffer.resize(size);
    }

    auto copy_next = [buf_iter = ranges::begin(buffer)](const auto& val) mutable {
        auto byte_array = std::bit_cast<std::array<Byte, sizeof(val)>>(val);
        detail::maybe_reverse_endianness<Endianness>(byte_array);
        ranges::copy_n(byte_array.begin(), byte_array.size(), buf_iter);
        buf_iter += byte_array.size();
    };

    ranges::for_each(range, copy_next);

    return buffer;
}

/**
 * \overload
 * \tparam NativeTo a \ref NativeByteVector class template
 */
template <std::same_as<NativeByteVector> NativeTo, ranges::range Range>
    requires(std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    return to_bytes<ByteVector, EndiannessKind::Native>(range);
}

/**
 * \overload
 * \tparam SizedTo a ByteArray class template
 * \tparam Size the size of the ByteArray
 */
template <template <std::size_t, EndiannessKind> typename SizedTo, std::size_t Size, EndiannessKind Endianness,
          ranges::range Range>
    requires(std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    return to_bytes<detail::partial_bytearray_spec<SizedTo, Size>::template templated, Endianness>(range);
}

/**
 * \overload
 */
template <template <std::size_t, EndiannessKind> typename SizedTo, EndiannessKind Endianness, ranges::range Range>
    requires(std::is_trivially_copyable_v<ranges::range_value_t<Range>> &&
             std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    static_assert(HasStaticSize<Range>, "Range must have a statically accessible size in order"
                                        " to infer size of ByteArray");

    constexpr auto size = sizeof(ranges::range_value_t<Range>) * get_static_size<Range>();

    return to_bytes<SizedTo, size, Endianness>(range);
}

/**
 * \overload
 * \tparam NativeSizedTo a \ref NativeByteArray class template
 */
template <template <std::size_t> typename NativeSizedTo, std::size_t Size, ranges::range Range>
    requires(std::same_as<NativeSizedTo<Size>, NativeByteArray<Size>> &&
             std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    return to_bytes<ByteArray, Size, EndiannessKind::Native>(range);
}

/**
 * \overload
 */
template <template <std::size_t> typename NativeSizedTo, ranges::range Range>
    requires(std::same_as<NativeSizedTo<0>, NativeByteArray<0>> &&
             std::is_trivially_copyable_v<ranges::range_value_t<Range>>)
constexpr auto to_bytes(const Range& range) {
    static_assert(HasStaticSize<Range>, "Range must have a statically accessible size in order"
                                        " to infer size of NativeByteArray");

    return to_bytes<ByteArray, EndiannessKind::Native>(range);
}

/**
 * \brief Reinterpret trivially copyable value(s) as a ByteVector or ByteArray, specifications ditto
 *
 * \overload
 * \tparam Ts the types of the values to reinterpret
 * \param values the values to reinterpret
 */
template <template <EndiannessKind> typename To, EndiannessKind Endianness, C<std::is_trivially_copyable>... Ts>
constexpr auto to_bytes(const Ts&... values) {
    To<Endianness> buffer{};

    if constexpr (requires(To<Endianness> rng, std::size_t new_size) { rng.resize(new_size); }) {
        constexpr auto size = (sizeof(Ts) + ...);
        buffer.resize(size);
    }

    auto copy_next = [buf_iter = ranges::begin(buffer)](const auto& val) mutable {
        auto byte_array = std::bit_cast<std::array<Byte, sizeof(val)>>(val);
        detail::maybe_reverse_endianness<Endianness>(byte_array);
        ranges::copy_n(byte_array.begin(), byte_array.size(), buf_iter);
        buf_iter += byte_array.size();
    };

    (copy_next(values), ...);

    return buffer;
}

/**
 * \overload
 */
template <std::same_as<NativeByteVector> NativeTo, C<std::is_trivially_copyable>... Ts>
constexpr auto to_bytes(const Ts&... values) {
    return to_bytes<ByteVector, EndiannessKind::Native>(values...);
}

/**
 * \overload
 */
template <template <std::size_t, EndiannessKind> typename SizedTo, std::size_t Size, EndiannessKind Endianness,
          C<std::is_trivially_copyable>... Ts>
constexpr auto to_bytes(const Ts&... values) {
    return to_bytes<detail::partial_bytearray_spec<SizedTo, Size>::template templated, Endianness>(values...);
}

/**
 * \overload
 */
template <template <std::size_t, EndiannessKind> typename SizedTo, EndiannessKind Endianness,
          C<std::is_trivially_copyable>... Ts>
constexpr auto to_bytes(const Ts&... values) {
    constexpr auto size = (0 + ... + sizeof(Ts));

    return to_bytes<SizedTo, size, Endianness>(values...);
}

/**
 * \overload
 */
template <template <std::size_t> typename NativeSizedTo, std::size_t Size, C<std::is_trivially_copyable>... Ts>
    requires(std::same_as<NativeSizedTo<Size>, NativeByteArray<Size>>)
constexpr auto to_bytes(const Ts&... values) {
    return to_bytes<ByteArray, Size, EndiannessKind::Native>(values...);
}

/**
 * \overload
 */
template <template <std::size_t N> typename NativeSizedTo, C<std::is_trivially_copyable>... Ts>
    requires(std::same_as<NativeSizedTo<0>, NativeByteArray<0>>)
constexpr auto to_bytes(const Ts&... values) {
    return to_bytes<ByteArray, EndiannessKind::Native>(values...);
}

/**
 * \brief callable providing a view adaptor to reinterpret a range of trivially copyable values as bytes.
 *
 * \see to_bytes
 */
template <EndiannessKind Endianness = EndiannessKind::Native>
constexpr decltype(auto) as_bytes() {
    return ranges::views::for_each([](const auto& val) { return to_bytes<ByteArray, Endianness>(val); });
}

} // namespace asmgrader
