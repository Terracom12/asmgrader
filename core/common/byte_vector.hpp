#pragma once

#include "common/aliases.hpp"

#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/list.hpp>
#include <gsl/assert>
#include <gsl/util>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/range/traits.hpp>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace asmgrader {

static_assert(ranges::input_iterator<const std::byte*>);

class ByteVector
{
public:
    // A bunch of aliases for use with algorithm templates that check for them
    using value_type = std::byte;
    using allocator_type = std::allocator<std::byte>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = std::vector<std::byte>::iterator;
    using const_iterator = std::vector<std::byte>::const_iterator;
    using reverse_iterator = std::vector<std::byte>::reverse_iterator;
    using const_reverse_iterator = std::vector<std::byte>::const_reverse_iterator;

    // Essentialy forwarding a lot of std::vector member functions
    ByteVector() = default;

    template <ranges::input_iterator It>
    ByteVector(It first, It last)
        : data_{first, last} {}

    ByteVector(std::initializer_list<std::byte> init)
        : data_{init} {}

    template <typename ByteLike = std::byte>
        requires requires(ByteLike value) { static_cast<std::byte>(value); }
    explicit ByteVector(std::size_t count, ByteLike value = std::byte{0})
        : data_{count, std::byte{value}} {}

    std::byte& operator[](size_t idx) { return data_[idx]; }

    const std::byte& operator[](size_t idx) const { return data_[idx]; }

    template <ranges::input_iterator It>
    auto insert(const_iterator pos, It first, It last) {
        return data_.insert(pos, first, last);
    }

    template <typename ByteLike>
        requires requires(ByteLike value) { static_cast<std::byte>(value); }
    void push_back(ByteLike value) {
        data_.push_back(value);
    }

    template <typename ByteLike>
        requires requires(ByteLike value) { static_cast<std::byte>(value); }
    void emplace_back(ByteLike value) {
        data_.emplace_back(value);
    }

    auto begin() { return data_.begin(); }

    auto begin() const { return data_.begin(); }

    auto cbegin() const { return data_.cend(); }

    auto end() { return data_.end(); }

    auto end() const { return data_.end(); }

    auto cend() const { return data_.cend(); }

    auto data() { return data_.data(); }

    auto data() const { return data_.data(); }

    size_t size() const { return data_.size(); }

    void resize(std::size_t new_size) { data_.resize(new_size); }

    // Extra ctors to convert from byte-like types (e.g., uint8_t)
    ByteVector(std::initializer_list<u8> init)
        : data_{init.size()} {
        init_range_to_bytes(init);
    }

    /// T should be a stdlib-compatible container type
    /// where std::byte is convertible to T::value_type
    template <ranges::range Range>
        requires requires(Range range, std::size_t size, std::byte byte) {
            { range.resize(size) };
            { std::to_integer<ranges::range_value_t<Range>>(byte) };
        }
    Range to_range() const {
        Range result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(),
                          [](std::byte value) { return std::to_integer<ranges::range_value_t<Range>>(value); });

        return result;
    }

    template <typename... Types>
        requires(std::is_trivially_copyable_v<Types> && ...)
    auto bit_cast_to() const
        -> std::conditional_t<sizeof...(Types) == 1, boost::mp11::mp_first<boost::mp11::mp_list<Types...>>,
                              std::tuple<Types...>> {
        constexpr auto TOTAL_SIZE = (sizeof(Types) + ...);
        Expects(TOTAL_SIZE <= size());

        std::array<std::byte, TOTAL_SIZE> bytes;
        ranges::copy(begin(), begin() + TOTAL_SIZE, bytes.begin());

        return {std::bit_cast<Types>(bytes)...};
    }

    template <ranges::range Range>
    static ByteVector from(const Range& range) {
        auto raw_bytes = std::as_bytes(std::span{range});

        static_assert(ranges::input_iterator<decltype(raw_bytes.begin())>);

        return {raw_bytes.begin(), raw_bytes.end()};
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
        requires requires(ranges::range_value_t<Range> value) { static_cast<std::byte>(value); }
    void init_range_to_bytes(const Range& range) {
        Expects(size() == ranges::size(range));

        ranges::transform(range, this->begin(), [](u8 value) { return std::byte{value}; });
    }

    std::vector<std::byte> data_;
};

} // namespace asmgrader
