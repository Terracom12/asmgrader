#pragma once

#include "common/formatters/formatters.hpp" // IWYU pragma: keep

#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <catch2/catch_tostring.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#include <string>

namespace Catch {

template <fmt::formattable T>
// inverse of Catch2's conditions as to not cause ambiguity
    requires(!is_range<T>::value && !Catch::Detail::IsStreamInsertable<T>::value)
struct StringMaker<T>
{
    static std::string convert(const T& t) { return fmt::format("{}", t); }
};

} // namespace Catch
