#pragma once

#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <catch2/catch_tostring.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#include <string>

namespace Catch {

template <fmt::formattable T>
struct StringMaker<T>
{
    static std::string convert(const T& t) { return fmt::format("{}", t); }
};

} // namespace Catch
