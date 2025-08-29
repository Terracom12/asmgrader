#pragma once

#include <asmgrader/common/byte_vector.hpp>
#include <asmgrader/subprocess/memory/memory_io_base.hpp>
#include <asmgrader/subprocess/memory/non_terminated_str.hpp>

#include <range/v3/algorithm/copy.hpp>

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

namespace asmgrader {

/// Example class implementation:
///
/// struct TraceeMemoryTypedIO<Type>
/// {
/// public:
///     static Result<T> read(std::uintptr_t address, TraceeMemory& mio);
///     static ByteVector to_bytes(const T& data);
/// };
template <typename T>
struct MemoryIOSerde;

namespace detail {

// FIXME: Some of this is probably UB

template <typename T>
ByteVector reinterpret_raw(const T& data) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* first = reinterpret_cast<const std::byte*>(&data);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return ByteVector{first, first + sizeof(data)};
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
        it = ranges::copy(reinterpret_raw(elem), it).out;
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
    static Result<T> read(std::uintptr_t address, MemoryIOBase& mio) {
        const auto raw_data = TRY(mio.read_block_impl(address, sizeof(T)));

        return raw_data.bit_cast_to<T>();
    }

    static ByteVector to_bytes(const T& data) { return detail::reinterpret_raw(data); }
};

template <>
struct MemoryIOSerde<std::string>
{
    static Result<std::string> read(std::uintptr_t address, MemoryIOBase& mio) {
        auto is_null_term = [](std::byte chr) { return std::to_integer<char>(chr) == '\0'; };
        const auto raw_data = TRY(mio.read_until(address, is_null_term));

        return data_to_str(raw_data);
    }

    static ByteVector to_bytes(const std::string& data) {
        // Include '\0'
        auto raw_data = ByteVector::from(data);
        raw_data.push_back(static_cast<std::byte>('\0'));

        return raw_data;
    }

    static std::string data_to_str(const ByteVector& data) { return data.to_range<std::string>(); }
};

template <std::size_t Length>
struct MemoryIOSerde<NonTermString<Length>>
{
    static Result<NonTermString<Length>> read(std::uintptr_t address, MemoryIOBase& mio) {
        const auto raw_data = TRY(mio.read_block_impl(address, Length));

        return {.string = MemoryIOSerde<std::string>::data_to_str(raw_data)};
    }

    static ByteVector to_bytes(const NonTermString<Length>& data) {
        return detail::reinterpret_raw_each(std::string_view{data.string, data.string + data.LENGTH});
    }
};

} // namespace asmgrader
