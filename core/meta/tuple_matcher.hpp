#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_rename.hpp>

#include <concepts>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace util {

namespace detail {

template <typename Ret, typename Func, typename T, typename... Ts>
constexpr Ret tuple_find_first_impl(Func&& pred, const T& val, const Ts&... rest) {
    if (std::invoke(std::forward<Func>(pred), val)) {
        return Ret{std::in_place, val};
    }

    if constexpr (sizeof...(Ts) > 0) {
        return tuple_find_first_impl<Ret>(std::forward<Func>(pred), rest...);
    }

    return std::nullopt;
}

} // namespace detail

template <typename Func, typename Tuple>
constexpr auto tuple_find_first(Func&& pred, const Tuple& val) {
    using VariantT = boost::mp11::mp_rename<boost::mp11::mp_unique<Tuple>, std::variant>;
    using Ret = std::optional<VariantT>;

    if constexpr (std::tuple_size_v<Tuple> == 0) {
        return Ret{std::nullopt};
    } else {
        return std::apply(
            [&pred](const auto&... elems) {
                return detail::tuple_find_first_impl<Ret>(std::forward<Func>(pred), elems...);
            },
            val);
    }
}

namespace tests {
using namespace std::literals;

static_assert(*tuple_find_first([](auto elem) { return elem == 1; }, std::make_tuple(0, 1)) == std::variant<int>{1});
static_assert(*tuple_find_first([](auto elem) { return elem == 0; }, std::make_tuple(0, 1)) == std::variant<int>{0});
static_assert(!tuple_find_first(
                   [](const auto& elem) {
                       if constexpr (std::same_as<std::decay_t<decltype(elem)>, int>) {
                           return elem == 3;
                       }
                       return false;
                   },
                   std::make_tuple(0, "Hi"))
                   .has_value());
static_assert(*tuple_find_first(
                  [](const auto& elem) {
                      if constexpr (std::same_as<std::decay_t<decltype(elem)>, int>) {
                          return elem == 0;
                      }
                      return false;
                  },
                  std::make_tuple(0, "Hi")) == std::variant<int, const char*>{0});
static_assert(*tuple_find_first(
                  [](const auto& elem) {
                      if constexpr (!std::same_as<std::decay_t<decltype(elem)>, int>) {
                          return elem == "bye";
                      }
                      return false;
                  },
                  std::make_tuple(0, "hi"sv, "bye"sv)) == std::variant<int, std::string_view>{"bye"});
} // namespace tests

} // namespace util
