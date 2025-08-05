#pragma once

#include <gsl/assert>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/iterator/traits.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/primitives.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

template <std::size_t Size>
class ByteArray : public std::array<std::byte, Size>
{
public:
    // // Conversion constructor from other containers holding "byte-like" objects (char, unsigned char)
    // template <typename T>
    //     requires requires(T::value_type value) {
    //         { static_cast<std::byte>(value) };
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

    /// T should be a stdlib-compatible container type
    /// where std::byte is convertible to T::value_type
    template <typename T>
        requires requires(T rng, std::size_t size, std::byte byte) {
            { rng.resize(size) };
            { std::to_integer<T::value_type>(byte) };
        }
    constexpr T to() const {
        T result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(), [](std::byte value) { return std::to_integer<T::value_type>(value); });

        return result;
    }

    // template <typename T>
    //     requires requires(T rng, std::size_t size, std::byte byte) {
    //         { rng.resize(size) };
    //         { static_cast<T::value_type>(byte) };
    //     }
    // constexpr T to() const {
    //     T result;
    //     result.resize(this->size());
    //
    //     ranges::transform(*this, result.begin(), [](std::byte value) { return static_cast<T::value_type>(value); });
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

        (ranges::copy(std::bit_cast<std::array<std::byte, sizeof(Ts)>>(args), std::exchange(it, it + sizeof(Ts))), ...);

        return result;
    }

private:
    template <ranges::range Range>
        requires requires(ranges::range_value_t<Range> value) {
            {
                std::byte {
                    value
                }
            };
        }
    constexpr void from_range(Range&& range) {
        // static_assert(get_size_if_constexpr(range) <= Size, "Passed range is too large for this ByteArray");
        Expects(ranges::size(range) < Size);
        ranges::transform(std::forward<Range>(range), this->begin(), [](std::byte value) { return std::byte{value}; });
    }

    static constexpr auto TRY_CONSTEXPR = []<std::size_t>() {};

    // template <ranges::range Range>
    //     requires requires { TRY_CONSTEXPR.template operator()<ranges::size(Range)>(); }
    // static constexpr std::size_t get_size_if_constexpr(const Range& range) {
    //     constexpr std::size_t NOT_CONSTEXPR = std::numeric_limits<std::size_t>::max();
    //
    //     if constexpr () {
    //         return ranges::size(range);
    //     }
    //
    //     return NOT_CONSTEXPR;
    // }
};

static_assert(std::is_aggregate_v<ByteArray<1>>);
