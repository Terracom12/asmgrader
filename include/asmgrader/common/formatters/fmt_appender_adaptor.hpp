/// \file
/// fmt::appender is non-comformat to the std iterator specification, and thus does
/// not work with the enourmously helpful range-v3 library. This file provides the
/// shim adaptor, ``fmt_appender_wrapper``, to permit this usage.
#pragma once

#include <fmt/base.h>

#include <cstddef>
#include <iterator>

template <typename CharType = char>
// NOLINTNEXTLINE(readability-identifier-naming) - rational: follow {fmt} conventions
struct fmt_appender_wrapper
{
    fmt::basic_appender<CharType> out;

    using difference_type = std::ptrdiff_t;
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    fmt_appender_wrapper& operator*() { return *this; }

    fmt_appender_wrapper& operator++() { return *this; }

    fmt_appender_wrapper& operator++(int) { return *this; }

    template <typename T>
    fmt_appender_wrapper& operator=(const T& value) {
        out = value;
        return *this;
    }
};
