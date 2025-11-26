#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/expected.hpp>

#include <charconv>
#include <concepts>
#include <string_view>
#include <system_error>

namespace asmgrader {

enum class IntParseError { Unknown, InvalidInput, OutOfRange };

template <std::integral IntType = i64>
Expected<IntType, IntParseError> parse_int(std::string_view str, int base = 10) {
    IntType res;
    std::from_chars_result conv_res = std::from_chars(str.data(), str.data() + str.size(), res, base);

    if (conv_res.ec == std::errc{}) {
        return res;
    }
    if (conv_res.ec == std::errc::invalid_argument) {
        return IntParseError::InvalidInput;
    }
    if (conv_res.ec == std::errc::result_out_of_range) {
        return IntParseError::OutOfRange;
    }

    return IntParseError::Unknown;
}

} // namespace asmgrader
