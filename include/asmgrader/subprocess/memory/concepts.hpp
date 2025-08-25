#pragma once

#include <asmgrader/common/byte_vector.hpp>
#include <asmgrader/common/error_types.hpp>
#include <asmgrader/subprocess/memory/non_terminated_str.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace asmgrader {

class MemoryIOBase;
template <typename T>
struct MemoryIOSerde;

// TODO: If efficiency with ptrace becomes an issue, consider an implementation with mmap

template <typename T>
concept MemoryReadSupported = requires(std::uintptr_t address, MemoryIOBase& mio) {
    { MemoryIOSerde<T>::read(address, mio) } -> std::same_as<Result<T>>;
};
template <typename T>
concept MemoryWriteSupported = requires(const T& data) {
    { MemoryIOSerde<T>::to_bytes(data) } -> std::same_as<ByteVector>;
};
template <typename T>
concept MemoryRWSupported = MemoryReadSupported<T> && MemoryWriteSupported<T>;

namespace detail {

// TODO: Ranges & byte array compat
// All string types are compatible
template <typename T, typename U>
struct MemoryIOCompatImpl : std::false_type
{
};

template <typename T>
concept CharLike = sizeof(T) == 1 &&
                   (std::is_fundamental_v<std::remove_cvref_t<T>> || std::same_as<std::remove_cvref_t<T>, std::byte>);

template <CharLike U>
struct MemoryIOCompatImpl<std::string, U*> : std::true_type
{
};

template <CharLike U>
struct MemoryIOCompatImpl<std::string_view, U*> : std::true_type
{
};

// Pointers and std::uintptr_t are compatible
template <typename U>
struct MemoryIOCompatImpl<std::uintptr_t, U*> : std::true_type
{
};

template <typename T, typename U>
struct MemoryIOCompat
{
    using NormT = std::decay_t<T>;
    using NormU = std::decay_t<U>;

    static constexpr bool value =
        (std::is_trivially_constructible_v<NormT, NormU> && std::is_trivially_constructible_v<NormU, NormT>) ||
        MemoryIOCompatImpl<NormT, NormU>::value || MemoryIOCompatImpl<NormU, NormT>::value;
};

} // namespace detail

/// A trait for types that are compatible in how they are read and written to memory
///
/// Permits for niceties like passing a `std::string` to an AsmFunction that accepts
/// a `const char*` parameter.
template <typename T, typename U>
concept MemoryIOCompatible = detail::MemoryIOCompat<T, U>::value;

static_assert(MemoryIOCompatible<std::string, char*>);
static_assert(MemoryIOCompatible<std::string_view, const char*>);
static_assert(MemoryIOCompatible<std::string_view, decltype("Hello")>);

static_assert(MemoryIOCompatible<int*, std::uintptr_t>);
static_assert(MemoryIOCompatible<const int*, std::uintptr_t>);

} // namespace asmgrader
