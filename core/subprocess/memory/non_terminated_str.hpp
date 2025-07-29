#pragma once

#include <cstddef>

// A non-null-terminated string for serialization/deserialization
template <std::size_t Length>
struct NonTermString
{
    const char* string{};
    constexpr static auto LENGTH = Length;
};
