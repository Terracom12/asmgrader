#pragma once

#include "boost/range/algorithm/copy.hpp"
#include "subprocess/memory/memory_io_base.hpp"
#include "subprocess/memory/non_terminated_str.hpp"
#include "util/byte_vector.hpp"

#include <boost/range/algorithm/transform.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

/// Example class implementation:
///
/// struct TraceeMemoryTypedIO<Type>
/// {
/// public:
///     static util::Result<T> read(std::uintptr_t address, TraceeMemory& mio);
///     static ByteVector to_bytes(const T& data);
/// };
template <typename T>
struct MemoryIOSerde;

namespace detail {

template <typename T>
ByteVector reinterpret_raw(const T& data) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* first = reinterpret_cast<const std::byte*>(&data);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return {first, first + sizeof(data)};
}

template <typename T>
const T* reinterpret_raw(const std::vector<std::byte>& data) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<const T*>(data.data());
}

template <typename T>
ByteVector reinterpret_raw_each(const T& range) {
    constexpr std::size_t VALUE_SIZE = sizeof(typename T::value_type);
    const std::size_t size = std::size(range) * VALUE_SIZE;
    ByteVector result(size);

    for (auto it = result.begin(); const auto& elem : range) {
        it = boost::range::copy(reinterpret_raw(elem), it);
    }

    return result;
}

} // namespace detail

template <>
struct MemoryIOSerde<ByteVector>
{
    static ByteVector to_bytes(const ByteVector& data) { return data; }
};

// "Raw" conversions (essentially just reinterpret_cast)
template <typename T>
    requires std::is_arithmetic_v<T> || std::same_as<T, std::timespec> || std::is_pointer_v<T> ||
             std::is_bounded_array_v<T> || (std::is_aggregate_v<T> && !std::is_array_v<T>)
struct MemoryIOSerde<T>
{
    static util::Result<T> read(std::uintptr_t address, MemoryIOBase& mio) {
        const auto raw_data = TRY(mio.read_block_impl(address, sizeof(T)));

        return *detail::reinterpret_raw<T>(raw_data);
    }

    static ByteVector to_bytes(const T& data) { return detail::reinterpret_raw(data); }
};

template <>
struct MemoryIOSerde<std::string>
{
    static util::Result<std::string> read(std::uintptr_t address, MemoryIOBase& mio) {
        auto is_null_term = [](std::byte chr) { return std::to_integer<char>(chr) == '\0'; };
        const auto raw_data = TRY(mio.read_until(address, is_null_term));

        return data_to_str(raw_data);
    }

    static ByteVector to_bytes(const std::string& data) {
        // Include '\0'
        auto raw_data = detail::reinterpret_raw_each(data);
        raw_data.push_back(static_cast<std::byte>('\0'));

        return raw_data;
    }

    static std::string data_to_str(const std::vector<std::byte>& data) {
        std::string result;
        result.resize(data.size());
        boost::range::transform(data, result.begin(), [](std::byte byte) { return std::to_integer<char>(byte); });
        return result;
    }
};

template <std::size_t Length>
struct MemoryIOSerde<NonTermString<Length>>
{
    static util::Result<NonTermString<Length>> read(std::uintptr_t address, MemoryIOBase& mio) {
        const auto raw_data = TRY(mio.read_block_impl(address, Length));

        return {.string = MemoryIOSerde<std::string>::data_to_str(raw_data)};
    }

    static ByteVector to_bytes(const NonTermString<Length>& data) {
        return detail::reinterpret_raw_each(std::string_view{data.string, data.string + data.LENGTH});
    }
};
