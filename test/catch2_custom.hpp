#pragma once

#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <fmt/format.h>

#pragma GCC diagnostic ignored "-Wunused-result"

namespace Catch {

template <fmt::formattable T>
struct StringMaker<T>
{
    static std::string convert(const T& t) { return fmt::format("{}", t); }
};

} // namespace Catch
