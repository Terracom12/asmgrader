#pragma once

#include "logging.hpp"
#include "util/extra_formatters.hpp"

#include <fmt/format.h>

#include <concepts>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>

namespace util {

template <typename T, typename E>
    requires(!std::is_convertible_v<T, E> && !std::is_convertible_v<T, E>)
/**
 * @brief std::variant wrapper for a partial implementation of C++23's expected type
 *
 * @tparam T The expected value type
 * @tparam E The error type
 *
 * Note: types T and E must not be convertible between one another.
 */
class Expected
{
public:
    using ExpectedT = T;
    using ErrT = E;

    template <typename Tu>
    constexpr Expected(Tu&& value) // NOLINT(*-explicit-*)
        requires(std::convertible_to<Tu, T>)
        : data_{std::forward<Tu>(value)} {}

    template <typename Eu>
    constexpr Expected(Eu&& error) // NOLINT(*-explicit-*)
        requires(std::convertible_to<Eu, E>)
        : data_{std::forward<Eu>(error)} {}

    constexpr void assert_val() { assert_expected(*this); }

    constexpr bool has_value() const { return std::holds_alternative<T>(data_); }
    constexpr bool has_error() const { return !has_value(); }
    constexpr explicit operator bool() const { return has_value(); }

    constexpr T& value() { return const_cast<T&>(const_cast<const Expected*>(this)->value()); }
    constexpr const T& value() const {
        if (!has_value()) {
            DEBUG_ASSERT(has_value(), "Bad expected access");
        }
        return std::get<T>(data_);
    }

    template <typename Tu>
    constexpr T value_or(Tu&& default_value) const {
        if (!has_value()) {
            return static_cast<T>(std::forward<Tu>(default_value));
        }
        return std::get<T>(data_);
    }

    constexpr E error() const {
        if (has_value()) {
            DEBUG_ASSERT(!has_value(), "Bad expected error access");
        }
        return std::get<E>(data_);
    }

    template <typename Eu>
    constexpr E error_or(Eu&& default_value) const {
        if (has_value()) {
            return static_cast<E>(std::forward<Eu>(default_value));
        }
        return std::get<E>(data_);
    }

    template <typename Func>
    constexpr Expected<std::invoke_result_t<Func, T>, E> transform(const Func& func) {
        if (!has_value()) {
            return error();
        }

        return func(value());
    }

private:
    std::variant<T, E> data_;
};

template <typename E>
/**
 * @brief std::variant wrapper for a partial implementation of C++23's expected type
 *
 * Specialization for T = void, a simple good / error type.
 *
 * @tparam T The expected value type
 * @tparam E The error type
 *
 * Note: types T and E must not be convertible between one another.
 */
class Expected<void, E>
{
public:
    constexpr Expected() // NOLINT(*-explicit-*)
        : data_{std::nullopt} {}

    template <typename Eu>
    constexpr Expected(Eu&& error) // NOLINT(*-explicit-*)
        requires(std::convertible_to<Eu, E>)
        : data_{std::forward<Eu>(error)} {}

    constexpr void assert_val() { assert_expected(*this); }

    constexpr bool has_value() const {
        // This might look strange, but the optional member holds an "error state",
        // so ``has_value`` wants to know if the optional is *empty* (i.e., no error!)
        return !data_.has_value();
    }
    constexpr bool has_error() const { return !has_value(); }
    constexpr explicit operator bool() const { return has_value(); }

    constexpr E error() const {
        if (!has_value()) {
            DEBUG_ASSERT(!has_value(), "Bad expected error access");
        }
        return data_.value();
    }

    template <typename Eu>
    constexpr E error_or(Eu&& default_value) const {
        if (has_value()) {
            return static_cast<E>(std::forward<Eu>(default_value));
        }
        return data_.value();
    }

private:
    std::optional<E> data_;
};

namespace expected_ct_tests {
using namespace std::string_view_literals;

static_assert(Expected<int, const char*>(100).value() == 100);
static_assert(Expected<int, const char*>("ERROR").error() == "ERROR"sv);

static_assert(Expected<int, const char*>(100).error_or("No error") == "No error"sv);
static_assert(Expected<int, const char*>("ERROR").value_or(-1) == -1);

static_assert(Expected<void, const char*>().error_or("No error") == "No error"sv);
static_assert(Expected<void, const char*>("ERROR").error() == "ERROR"sv);

consteval Expected<std::array<int, 10>, int> aggregate_expected_test(bool error) {
    if (error) {
        return -1;
    }

    return std::array<int, 10>{1, 2, 3, 4, 5};
}
static_assert(aggregate_expected_test(false).value() == std::array<int, 10>{1, 2, 3, 4, 5});
static_assert(aggregate_expected_test(true).error() == -1);

} // namespace expected_ct_tests

template <typename T, typename E>
inline auto format_as(const Expected<T, E>& from) {
    if (!from) {
        return fmt::format("Error({})", from.error());
    }

    return fmt::format("Expected({})", from.value());
}

template <typename E>
inline auto format_as(const Expected<void, E>& from) {
    if (!from) {
        return fmt::format("Error({})", from.error());
    }

    return fmt::format("Expected(void)");
}

template <typename T, typename E>
#ifndef DEBUG
#define ASSERT_EXPECTED_DEPRECATED [[deprecated("Do not use ASSERT_EXPECTED in non-debug mode")]]
#else
#define ASSERT_EXPECTED_DEPRECATED
#endif

[[maybe_unused]] ASSERT_EXPECTED_DEPRECATED inline void assert_expected(const util::Expected<T, E>& exp,
                                                                        std::string_view msg = "") {
    if (exp.has_value()) {
        return;
    }
    if constexpr (fmt::formattable<E>) {
        LOG_FATAL("Erroneous unexpected value ({}): {:?}", exp.error(), msg);
    } else {
        LOG_FATAL("Erroneous unexpected value (<not formattable>): {:?}", msg);
    }

    _exit(1);
}

} // namespace util
