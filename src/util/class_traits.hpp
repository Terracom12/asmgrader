#pragma once

#include <chrono>
#include <type_traits>

namespace util {

/**
 * \brief A trivially-movable, but non-copyable type.
 *
 * Use as a superclass to annotate a subclass as non-copyable.
 */
class NonCopyable
{
public:
    NonCopyable() = default;

    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(const NonCopyable&) = delete;

    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;

    ~NonCopyable() = default;
};

static_assert(std::is_trivially_constructible_v<NonCopyable> && !std::is_copy_assignable_v<NonCopyable> &&
                  !std::is_copy_constructible_v<NonCopyable> && std::is_trivially_move_assignable_v<NonCopyable> &&
                  std::is_trivially_move_constructible_v<NonCopyable>,
              "Class non_copyable should be trivially constructible and movable, not copyable");

/**
 * \brief A non-movable and non-copyable type.
 *
 * Use as a superclass to annotate a subclass as non-copyable.
 */
class NonMovable : public NonCopyable
{
public:
    NonMovable() = default;

    NonMovable(const NonMovable&) = delete;
    NonMovable& operator=(const NonMovable&) = delete;

    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;

    ~NonMovable() = default;
};
static_assert(std::is_trivially_constructible_v<NonMovable> && std::is_trivially_copyable_v<NonMovable> &&
                  !std::is_copy_assignable_v<NonMovable> && !std::is_copy_constructible_v<NonMovable> &&
                  !std::is_move_assignable_v<NonMovable> && !std::is_move_constructible_v<NonMovable>,
              "Class non_movable should be trivially constructible, but neither copyable nor movable");

// Credit to: https://stackoverflow.com/a/77263105
template <typename T>
concept ChronoDuration = requires {
    []<class Rep, class Period>(std::type_identity<std::chrono::duration<Rep, Period>>) {}(std::type_identity<T>());
};

} // namespace util
