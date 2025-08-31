#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/byte.hpp>

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

class ByteVector
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
    using iterator = std::vector<Byte>::iterator;
    using const_iterator = std::vector<Byte>::const_iterator;
    using reverse_iterator = std::vector<Byte>::reverse_iterator;
    using const_reverse_iterator = std::vector<Byte>::const_reverse_iterator;

    // Essentialy forwarding a lot of std::vector member functions
    ByteVector() = default;

    template <ranges::input_iterator It>
    ByteVector(It first, It last)
        : data_{first, last} {}

    ByteVector(std::initializer_list<Byte> init)
        : data_{init} {}

    explicit ByteVector(std::size_t count, Byte value = Byte{})
        : data_(count, Byte{value}) {}

    Byte& operator[](size_t idx) { return data_[idx]; }

    const Byte& operator[](size_t idx) const { return data_[idx]; }

    template <ranges::input_iterator It>
    auto insert(const_iterator pos, It first, It last) {
        return data_.insert(pos, first, last);
    }

    template <typename ByteLike>
        requires requires(ByteLike value) { static_cast<Byte>(value); }
    void push_back(ByteLike value) {
        data_.push_back(value);
    }

    template <typename ByteLike>
        requires requires(ByteLike value) { static_cast<Byte>(value); }
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

    /// T should be a stdlib-compatible container type
    /// where Byte is convertible to T::value_type
    template <ranges::range Range>
        requires requires(Range range, std::size_t size, Byte byte) {
            { range.resize(size) };
            { static_cast<ranges::range_value_t<Range>>(byte.value) };
        }
    Range to_range() const {
        Range result;
        result.resize(this->size());

        ranges::transform(*this, result.begin(),
                          [](Byte byte) { return static_cast<ranges::range_value_t<Range>>(byte.value); });

        return result;
    }

    template <typename... Types>
        requires(std::is_trivially_copyable_v<Types> && ...)
    auto bit_cast_to() const
        -> std::conditional_t<sizeof...(Types) == 1, boost::mp11::mp_first<boost::mp11::mp_list<Types...>>,
                              std::tuple<Types...>> {
        constexpr auto TOTAL_SIZE = (sizeof(Types) + ...);
        Expects(TOTAL_SIZE <= size());

        std::array<Byte, TOTAL_SIZE> bytes;
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

        (ranges::copy(std::bit_cast<std::array<Byte, sizeof(Ts)>>(args), std::exchange(it, it + sizeof(Ts))), ...);

        return result;
    }

private:
    template <ranges::range Range>
        requires requires(ranges::range_value_t<Range> value) { static_cast<Byte>(value); }
    void init_range_to_bytes(const Range& range) {
        Expects(size() == ranges::size(range));

        ranges::transform(range, this->begin(), [](u8 value) { return Byte{value}; });
    }

    std::vector<Byte> data_;
};

} // namespace asmgrader
