#pragma once

#include <gsl/assert>
#include <gsl/util>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/traits.hpp>

#include <cstddef>
#include <cstdint>
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
    explicit ByteVector(const T& container)
        : std::vector<std::byte>{container.size()} {
        init_range_to_bytes(container);
    }

    // FIXME: Duplicated code
    ByteVector(std::initializer_list<std::uint8_t> init)
        : std::vector<std::byte>{init.size()} {
        init_range_to_bytes(init);
    }

    /// T should be a stdlib-compatible container type
    /// where std::byte is convertible to T::value_type
    template <ranges::range Range>
        requires requires(Range range, std::size_t size, std::byte byte) {
            { range.resize(size) };
            { std::to_integer<ranges::range_value_t<Range>>(byte) };
        }
    Range to() const {
        Range result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(),
                          [](std::byte value) { return std::to_integer<ranges::range_value_t<Range>>(value); });

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

private:
    template <ranges::range Range>
        requires requires(ranges::range_value_t<Range> value) { std::byte{value}; }
    void init_range_to_bytes(const Range& range) {
        Expects(size() == ranges::size(range));

        ranges::transform(range, this->begin(), [](std::uint8_t value) { return std::byte{value}; });
    }
};
