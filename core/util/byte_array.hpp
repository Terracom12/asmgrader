#pragma once

#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/concepts.hpp>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

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
    // // FIXME: Duplicated code
    // template <typename T>
    //     requires requires(T value) {
    //         { static_cast<std::byte>(value) };
    //     }
    // constexpr ByteArray(std::initializer_list<T> init)
    //     : ByteArray{init} {
    //     static_assert(init.size() <= Size, "Initializer list is to large to fit into this ByteArray");
    // }

    /// T should be a stdlib-compatible container type
    /// where std::byte is convertible to T::value_type
    template <typename T>
        requires requires(T rng, std::size_t size, std::byte byte) {
            { rng.resize(size) };
            { static_cast<T::value_type>(byte) };
        }
    constexpr T to() const {
        T result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(), [](std::byte value) { return static_cast<T::value_type>(value); });

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

    template <typename T>
    static constexpr ByteArray from(std::span<const T> elems) {
        auto raw_bytes = elems.as_bytes();

        return ByteArray{raw_bytes.begin(), raw_bytes.end()};
    }

    template <typename... Ts>
    static constexpr ByteArray from(const Ts&... args) {
        ByteArray result((sizeof(Ts) + ...));

        auto it = ranges::begin(result);

        (ranges::copy(std::bit_cast<std::array<std::byte, sizeof(Ts)>>(args), std::exchange(it, it + sizeof(Ts))), ...);

        return result;
    }

private:
    // template <ranges::range Range>
    // constexpr ByteArray(Range&& range) {
    //     ranges::transform(std::forward<Range>(range), this->begin(),
    //                       [](std::byte value) { return static_cast<std::byte>(value); });
    // }
};

static_assert(std::is_aggregate_v<ByteArray<1>>);
