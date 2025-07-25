#pragma once

#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>

#include <cstddef>
#include <initializer_list>
#include <span>
#include <utility>
#include <vector>

class ByteVector : public std::vector<std::byte>
{
public:
    using std::vector<std::byte>::vector;

    // Conversion constructor from other containers holding "byte-like" objects (char, unsigned char)
    template <typename T>
        requires requires(T::value_type value) {
            { static_cast<std::byte>(value) };
        }
    explicit ByteVector(const T& container)
        : std::vector<std::byte>{container.size()} {
        ranges::transform(container, this->begin(), [](T::value_type value) { return static_cast<std::byte>(value); });
    }

    // FIXME: Duplicated code
    template <typename T>
        requires requires(T value) {
            { static_cast<std::byte>(value) };
        }
    ByteVector(std::initializer_list<T> init)
        : std::vector<std::byte>{init.size()} {
        ranges::transform(init, this->begin(), [](T value) { return static_cast<std::byte>(value); });
    }

    /// T should be a stdlib-compatible container type
    /// where std::byte is convertible to T::value_type
    template <typename T>
        requires requires(T rng, std::size_t size, std::byte byte) {
            { rng.resize(size) };
            { static_cast<T::value_type>(byte) };
        }
    T to() const {
        T result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(), [](std::byte value) { return static_cast<T::value_type>(value); });

        return result;
    }

    template <typename T>
    static ByteVector from(std::span<const T> elems) {
        auto raw_bytes = elems.as_bytes();

        return ByteVector{raw_bytes.begin(), raw_bytes.end()};
    }

    template <typename... Ts>
    static ByteVector from(const Ts&... args) {
        ByteVector result((sizeof(Ts) + ...));

        auto it = ranges::begin(result);

        (ranges::copy(std::bit_cast<std::array<std::byte, sizeof(Ts)>>(args), std::exchange(it, it + sizeof(Ts))), ...);

        return result;
    }
};
