#pragma once

#include <asmgrader/common/byte.hpp>

#include <gsl/assert>
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
#include <span>
#include <type_traits>
#include <utility>

namespace asmgrader {

template <std::size_t Size>
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

    // // Conversion constructor from other containers holding "byte-like" objects (char, unsigned char)
    // template <typename T>
    //     requires requires(T::value_type value) {
    //         { static_cast<Byte>(value) };
    //     }
    // explicit constexpr from(const T& container)
    //     : ByteArray{container} {
    //     static_assert(container.size() <= Size, "Container is to large to fit into this ByteArray");
    // }
    //
    // template <ranges::range Range>
    // explicit constexpr ByteArray(Range&& range) {
    //     Expects(range.size() <= Size);
    //
    //     from_range(std::forward<Range>(range));
    // }
    //

    auto begin() { return data.begin(); }

    auto begin() const { return data.begin(); }

    auto cbegin() const { return data.cend(); }

    auto end() { return data.end(); }

    auto end() const { return data.end(); }

    auto cend() const { return data.cend(); }

    std::size_t size() const { return data.size(); }

    /// T should be a stdlib-compatible container type
    /// where Byte is convertible to T::value_type
    template <ranges::range Range>
        requires requires(Range rng, std::size_t size, Byte byte) {
            { rng.resize(size) };
            { static_cast<ranges::range_value_t<Range>>(byte.value) };
        }
    constexpr Range to() const {
        Range result;
        result.resize(size());

        ranges::transform(*this, result.begin(),
                          [](Byte byte) { return static_cast<ranges::range_value_t<Range>>(byte.value); });

        return result;
    }

    // template <typename T>
    //     requires requires(T rng, std::size_t size, Byte byte) {
    //         { rng.resize(size) };
    //         { static_cast<T::value_type>(byte) };
    //     }
    // constexpr T to() const {
    //     T result;
    //     result.resize(this->size());
    //
    //     ranges::transform(*this, result.begin(), [](Byte value) { return static_cast<T::value_type>(value); });
    //
    //     return result;
    // }

    template <ranges::range Range>
    static constexpr ByteArray bit_cast_range(Range&& range) {
        auto raw_bytes = std::as_bytes(
            std::span{ranges::begin(std::forward<Range>(range)), ranges::end(std::forward<Range>(range))});

        ByteArray<Size> res{};
        res.from_range(raw_bytes);
        return res;
    }

    template <typename... Ts>
    static constexpr ByteArray bit_cast(const Ts&... args) {
        ByteArray result{};

        auto it = ranges::begin(result);

        (ranges::copy(std::bit_cast<std::array<Byte, sizeof(Ts)>>(args), std::exchange(it, it + sizeof(Ts))), ...);

        return result;
    }

    std::array<Byte, Size> data;

private:
    template <ranges::range Range>
        requires requires(ranges::range_value_t<Range> value) {
            { Byte{value} };
        }
    constexpr void from_range(Range&& range) {
        // static_assert(get_size_if_constexpr(range) <= Size, "Passed range is too large for this ByteArray");
        Expects(ranges::size(range) < Size);
        ranges::transform(std::forward<Range>(range), data.begin(), [](Byte value) { return Byte{value}; });
    }
};

static_assert(std::is_aggregate_v<ByteArray<1>>);

} // namespace asmgrader
