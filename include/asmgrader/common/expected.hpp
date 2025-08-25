#pragma once

#include <asmgrader/common/extra_formatters.hpp>
#include <asmgrader/logging.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <concepts>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include <unistd.h> // _exit

namespace asmgrader {

struct UnexpectedT
{
};

template <typename T = void, typename E = std::error_code>
/**
 * @brief std::variant wrapper for a partial implementation of C++23's expected type
 *
 * @tparam T The expected value type
 * @tparam E The error type
 *
 * Note: types T and E must not be convertible between one another.
 */
class [[nodiscard]] Expected
{
public:
    using ExpectedT = T;
    using ErrT = E;

    constexpr Expected()
        : data_{} {}

    template <typename... Args>
    explicit constexpr Expected(std::in_place_t /*unused*/, Args&&... args)
        requires(!std::is_void_v<T> && std::constructible_from<ExpectedT, Args...>)
        : data_{std::in_place_type<ExpectedT>, std::forward<Args>(args)...} {}

    template <typename Tu>
    constexpr Expected(Tu&& value) // NOLINT(*-explicit-*)
        requires(!std::is_void_v<T> && std::convertible_to<Tu, T>)
        : data_{std::forward<Tu>(value)} {}

    template <typename Eu>
    constexpr Expected(Eu&& error) // NOLINT(*-explicit-*)
        requires(std::convertible_to<Eu, E> && !std::is_convertible_v<T, E> && !std::is_convertible_v<T, E>)
        : data_{std::forward<Eu>(error)} {}

    template <typename Eu>
    constexpr Expected(UnexpectedT /*unused*/, Eu&& error) // NOLINT(*-explicit-*)
        requires(std::convertible_to<Eu, E>)
        : data_{std::forward<Eu>(error)} {}

    constexpr void assert_val() { assert_expected(*this); }

    constexpr bool has_value() const {
        if constexpr (std::is_void_v<T>) {
            return std::holds_alternative<std::monostate>(data_.data);
        } else {
            return std::holds_alternative<T>(data_.data);
        }
    }

    constexpr bool has_error() const { return !has_value(); }

    constexpr explicit operator bool() const { return has_value(); }

    // TODO: clean this up. It seems like a whole void specialization would be required, but there MUST be a better way
    template <typename U = T>
    constexpr U& value()
        requires(!std::is_void_v<U>)
    {
        return const_cast<U&>(const_cast<const Expected*>(this)->value());
    }

    template <typename U = T>
    constexpr const U& value() const
        requires(!std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::value() for any type other than T");
        ASSERT_NOLOG(has_value());
        return std::get<U>(data_.data);
    }

    template <typename U = T>
    constexpr void value() const
        requires(std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::value() for any type other than T");
        ASSERT_NOLOG(has_value());
    }

    template <typename U = T>
    constexpr U& operator*()
        requires(!std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::operator*() for any type other than T");
        return value();
    }

    template <typename U = T>
    constexpr const U& operator*() const
        requires(!std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::operator*() for any type other than T");
        return value();
    }

    template <typename U = T>
    constexpr U* operator->()
        requires(!std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::operator->() for any type other than T");
        return &value();
    }

    template <typename U = T>
    constexpr const U* operator->() const
        requires(!std::is_void_v<U>)
    {
        static_assert(std::same_as<U, T>,
                      "Do not attempt to instantiate Expected<T,E>::operator->() for any type other than T");
        return &value();
    }

    template <typename Tu>
    constexpr T value_or(Tu&& default_value) const
        requires(std::convertible_to<Tu, T>)
    {
        if (!has_value()) {
            return static_cast<T>(std::forward<Tu>(default_value));
        }
        return std::get<T>(data_.data);
    }

    constexpr E error() const {
        ASSERT_NOLOG(!has_value());
        return std::get<E>(data_.data);
    }

    template <typename Eu>
    constexpr E error_or(Eu&& default_value) const {
        if (has_value()) {
            return static_cast<E>(std::forward<Eu>(default_value));
        }
        return std::get<E>(data_.data);
    }

    template <typename Func>
    constexpr Expected<std::invoke_result_t<Func, T>, E> transform(const Func& func) {
        if (!has_value()) {
            return error();
        }

        return func(value());
    }

private:
    template <typename Td, typename Ed>
    struct ExpectedData
    {
        std::variant<Td, Ed> data;
        constexpr auto operator<=>(const ExpectedData& rhs) const = default;
    };

    template <typename Ed>
    struct ExpectedData<void, Ed>
    {
        std::variant<std::monostate, Ed> data;
        constexpr auto operator<=>(const ExpectedData& rhs) const = default;
    };

public:
    constexpr auto operator<=>(const Expected& rhs) const = default;

    // TODO: Figure out why it's necessary to define this
    constexpr bool operator==(const Expected& rhs) const
        requires(std::equality_comparable<E> && (std::is_void_v<T> || std::equality_comparable<T>))
    {
        return data_ == rhs.data_;
    }

    template <typename Tu>
    constexpr bool operator==(const Tu& rhs) const
        requires(!std::is_void_v<T> && !std::same_as<Tu, Expected> && std::equality_comparable_with<Tu, T>)
    {
        if (!has_value()) {
            return false;
        }

        return value() == rhs;
    }

    template <typename Eu>
    constexpr bool operator==(const Eu& rhs) const
        requires(!std::same_as<Eu, Expected> && std::equality_comparable_with<Eu, E>)
    {
        if (has_value()) {
            return false;
        }

        return error() == rhs;
    }

private:
    ExpectedData<T, E> data_;
};

template <typename E>
inline auto format_as(const Expected<void, E>& from) {
    if (!from) {
        return fmt::format("Error({})", from.error());
    }

    return fmt::format("Expected(void)");
}

#ifndef DEBUG
#define ASSERT_EXPECTED_DEPRECATED [[deprecated("Do not use ASSERT_EXPECTED in non-debug mode")]]
#else
#define ASSERT_EXPECTED_DEPRECATED
#endif

template <typename T, typename E>
[[maybe_unused]] ASSERT_EXPECTED_DEPRECATED inline void assert_expected(const Expected<T, E>& exp,
                                                                        std::string_view msg = "<no message>") {
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

} // namespace asmgrader

template <typename T, typename E>
struct fmt::formatter<::asmgrader::Expected<T, E>> : ::asmgrader::DebugFormatter
{

    auto format(const ::asmgrader::Expected<T, E>& from, fmt::format_context& ctx) const {
        // TODO: Use a `copy` algo. `ranges::copy` has stricter iter reqs than the `ctx.out()` type impls
        return format_to(ctx.out(), "{}", format_impl(from));
    }

private:
    std::string format_impl(const ::asmgrader::Expected<T, E>& from) const {
        if (!from) {
            if constexpr (formattable<E>) {
                return fmt::format("Error({})", from.error());
            } else {
                return "Error(<unformattable>)";
            }
        }

        if constexpr (std::same_as<T, void>) {
            return "Expected(void)";
        } else if constexpr (formattable<T>) {
            return fmt::format("Expected({})", from.value());
        } else {
            return "Expected(<unformattable>)";
        }
    }
};
