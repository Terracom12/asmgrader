#pragma once

#include "subprocess/memory/non_terminated_str.hpp"
#include "util/byte_vector.hpp"
#include "util/error_types.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>

class MemoryIOBase;
template <typename T>
struct MemoryIOSerde;

// TODO: If efficiency with ptrace becomes an issue, consider an implementation with mmap

template <typename T>
concept MemoryReadSupportedType = requires(std::uintptr_t address, MemoryIOBase& mio) {
    { MemoryIOSerde<T>::read(address, mio) } -> std::same_as<util::Result<T>>;
};
template <typename T>
concept MemoryWriteSupportedType = requires(const T& data) {
    { MemoryIOSerde<T>::to_bytes(data) } -> std::same_as<ByteVector>;
};
template <typename T>
concept MemoryIOSupportedType = MemoryReadSupportedType<T> && MemoryWriteSupportedType<T>;

namespace detail {

template <typename T>
struct MemoryIODecay
{
    using type = T;
};

template <>
struct MemoryIODecay<std::string>
{
    using type = char*;
};

template <std::size_t Length>
struct MemoryIODecay<NonTermString<Length>> : MemoryIODecay<std::string>
{
};

template <>
struct MemoryIODecay<ByteVector>
{
    using type = char*;
};

} // namespace detail

template <typename T>
using MemoryIODecayed = detail::MemoryIODecay<T>::type;
