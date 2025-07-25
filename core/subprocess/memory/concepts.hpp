#pragma once

#include "util/byte_vector.hpp"

#include <concepts>
#include <cstdint>

class MemoryIOBase;
template <typename T>
struct MemoryIOSerde;

// TODO: If efficiency with ptrace becomes an issue, consider an implementation with mmap

template <typename T>
concept MemoryReadSupportedType = requires(std::uintptr_t address, MemoryIOBase& mio) {
    { MemoryIOSerde<T>::read(address, mio) } -> std::same_as<T>;
};
template <typename T>
concept MemoryWriteSupportedType = requires(const T& data) {
    { MemoryIOSerde<T>::to_bytes(data) } -> std::same_as<ByteVector>;
};
template <typename T>
concept MemoryIOSupportedType = MemoryReadSupportedType<T> && MemoryWriteSupportedType<T>;
