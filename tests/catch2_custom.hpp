#pragma once

#include "common/bit_casts.hpp"
#include "common/formatters/formatters.hpp" // IWYU pragma: keep
#include "common/os.hpp"

#include <boost/preprocessor/facilities/identity.hpp>
#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <catch2/catch_tostring.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/view.hpp>

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Catch {

template <typename View>
concept ImmutableView = ranges::range<View> && !requires(const View& view) {
    { *view.begin()++ };
};

using asmgrader::as_bytes;
static_assert(ImmutableView<decltype(std::declval<std::vector<int>&>() | as_bytes())>);

template <fmt::formattable T>
// inverse of Catch2's conditions as to not cause ambiguity
    requires(!ImmutableView<T> && !is_range<T>::value && !Catch::Detail::IsStreamInsertable<T>::value)
struct StringMaker<T>
{
    static std::string convert(const T& t) { return fmt::format("{}", t); }
};

// Example:
// REQUIRE_THAT(array | as_bytes(), RangeEquals(other));
template <fmt::formattable T>
    requires(ImmutableView<T> && !is_range<T>::value && !Catch::Detail::IsStreamInsertable<T>::value)
struct StringMaker<T>
{
    // Make a copy of an immutable view
    static std::string convert(T t) { return fmt::format("{}", t); }
};

} // namespace Catch

/// Like Catch2's `table`, but simpler and constexpr-capable
template <typename... Types, std::size_t N>
constexpr auto make_tests(const std::tuple<Types...> (&init)[N]) {
    return std::to_array(init);
}

#define IDENTITY(...) __VA_ARGS__
#define STRIP_PARENS(parens) IDENTITY parens

#define STATIC_TABLE_TESTS_IMPL(table, parenthesized_elems, fn_body)                                                   \
    []<std::size_t... I>(std::index_sequence<I...>) {                                                                  \
        (([] {                                                                                                         \
             const auto& [STRIP_PARENS(parenthesized_elems)] = table[I];                                               \
             [&] STRIP_PARENS(fn_body)();                                                                              \
         }()),                                                                                                         \
         ...);                                                                                                         \
    }(std::make_index_sequence<table.size()>())

/// Example usage:
///
/// STATIC_TABLE_TESTS( //
///     int_tests, (data, expected_msb, expected_lsb), ({
///         constexpr auto msb_bytes = to_bytes<ByteArray, Big>(data);
///         constexpr auto lsb_bytes = to_bytes<ByteArray, Little>(data);
///
///         STATIC_REQUIRE(ranges::equal(msb_bytes, expected_msb));
///         STATIC_REQUIRE(ranges::equal(lsb_bytes, expected_lsb));
///     }));
#define STATIC_TABLE_TESTS(table, parenthesized_elems, fn_body)                                                        \
    STATIC_TABLE_TESTS_IMPL(table, parenthesized_elems, fn_body)
