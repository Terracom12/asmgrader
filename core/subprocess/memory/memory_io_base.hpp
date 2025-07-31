#pragma once

#include "meta/remove_all_pointers.hpp"
#include "subprocess/memory/concepts.hpp"
#include "util/byte_vector.hpp"
#include "util/error_types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <type_traits>

#include <sys/types.h>

/// Base class for interacting with a tracee's memory in a variety of ways at a (relatively) high-level
/// Functionally a data pipeline between the tracer and tracee
///
/// A precondition to essentially every operation performed by this class is that the tracee is stopped
class MemoryIOBase
{
public:
    explicit MemoryIOBase(pid_t pid)
        : pid_{pid} {};
    virtual ~MemoryIOBase() = default;

    template <MemoryReadSupported T>
    util::Result<T> read(std::uintptr_t address) {
        return MemoryIOSerde<T>::read(address, *this);
    }

    template <MemoryReadSupported T>
        requires std::is_trivial_v<T> // more complex types may have sizes that don't correspond
    util::Result<std::vector<T>> read_array(std::uintptr_t address, std::size_t size) {
        std::vector<T> result;
        result.reserve(size);

        for (std::uintptr_t current_address = address; size > 0; current_address += sizeof(T), --size) {
            result.push_back(TRY(MemoryIOSerde<T>::read(current_address, *this)));
        }

        return result;
    }

    template <MemoryReadSupported T>
        requires std::is_trivial_v<T> // more complex types may have sizes that don't correspond
    util::Result<std::vector<T>> read_array(std::uintptr_t address, std::function<bool(const T&)> until_predicate) {
        std::vector<T> result;

        for (std::uintptr_t current_address = address;; current_address += sizeof(T)) {
            T elem = TRY(MemoryIOSerde<T>::read(current_address, *this));

            if (!until_predicate(elem)) {
                break;
            }

            result.push_back(std::move(elem));
        }

        return result;
    }

    util::Result<ByteVector> read_bytes(std::uintptr_t address, std::size_t length) {
        return this->read_block_impl(address, length);
    }

    // FIXME: The reinterpret_cast's to/from std::uintptr_t and pointer types seems kinda bad
    template <MemoryReadSupported T>
    util::Result<util::remove_all_pointers_t<T>> read_deref_all(std::uintptr_t address) {
        if constexpr (std::is_pointer_v<T>) {
            using NextType = std::remove_pointer_t<T>;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto next_address = reinterpret_cast<std::uintptr_t>(TRY(read<T>(address)));

            return TRY(read_deref_all<NextType>(next_address));
        } else {
            return TRY(read<T>(address));
        }
    }

    template <MemoryWriteSupported T>
    util::Result<std::size_t> write(std::uintptr_t address, const T& data) {
        ByteVector bytes = MemoryIOSerde<T>::to_bytes(data);
        TRY(this->write_block_impl(address, bytes));
        return bytes.size();
    }

    pid_t get_pid() const { return pid_; }

private:
    pid_t pid_;

    template <typename T>
    friend struct MemoryIOSerde;

    virtual util::Result<ByteVector> read_until(std::uintptr_t address,
                                                const std::function<bool(std::byte)>& predicate);
    virtual util::Result<ByteVector> read_until(std::uintptr_t address,
                                                const std::function<bool(std::span<const std::byte>)>& predicate,
                                                std::size_t block_size);

    virtual util::Result<ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) = 0;
    virtual util::Result<void> write_block_impl(std::uintptr_t address, const ByteVector& data) = 0;
};
