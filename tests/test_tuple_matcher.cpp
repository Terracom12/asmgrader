#include "catch2_custom.hpp"

#include "meta/tuple_matcher.hpp"

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

using asmgrader::tuple_find_first;

TEST_CASE("tuple_find_first compile-time tests") {

    using namespace std::literals;

    STATIC_REQUIRE(*tuple_find_first([](auto elem) { return elem == 1; }, std::make_tuple(0, 1)) ==
                   std::variant<int>{1});
    STATIC_REQUIRE(*tuple_find_first([](auto elem) { return elem == 0; }, std::make_tuple(0, 1)) ==
                   std::variant<int>{0});
    STATIC_REQUIRE(!tuple_find_first(
                        [](const auto& elem) {
                            if constexpr (std::same_as<std::decay_t<decltype(elem)>, int>) {
                                return elem == 3;
                            }
                            return false;
                        },
                        std::make_tuple(0, "Hi"))
                        .has_value());
    STATIC_REQUIRE(*tuple_find_first(
                       [](const auto& elem) {
                           if constexpr (std::same_as<std::decay_t<decltype(elem)>, int>) {
                               return elem == 0;
                           }
                           return false;
                       },
                       std::make_tuple(0, "Hi")) == std::variant<int, const char*>{0});
    STATIC_REQUIRE(*tuple_find_first(
                       [](const auto& elem) {
                           if constexpr (!std::same_as<std::decay_t<decltype(elem)>, int>) {
                               return elem == "bye";
                           }
                           return false;
                       },
                       std::make_tuple(0, "hi"sv, "bye"sv)) == std::variant<int, std::string_view>{"bye"});
}
