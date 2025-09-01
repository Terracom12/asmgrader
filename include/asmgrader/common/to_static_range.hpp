#pragma once

#include <asmgrader/meta/static_size.hpp>

#include <libassert/assert.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/iterator/access.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/range/traits.hpp>
#include <range/v3/view/view.hpp>

#include <array>
#include <cstddef>
#include <utility>

namespace asmgrader {

/// Range adaptor function to convert a range to a static (sized at compile time) container,
/// like std::array, std::tuple, etc.
template <ranges::range Container>
    requires(HasStaticSize<Container>)
struct to_static_container_fn
{
    template <ranges::input_range Rng>
        requires(ranges::sized_range<Rng>)
    constexpr auto operator()(Rng&& rng) const {
        // Ensure the range has exactly N elements in debug builds
        DEBUG_ASSERT(ranges::size(rng) == get_static_size<Container>());
        // In non-debug builds, be more lax and check that the container can fit within N elements
        ASSERT(ranges::size(rng) <= get_static_size<Container>());

        Container result{};
        ranges::copy(std::forward<Rng>(rng), result.begin());
        return result;
    }
};

template <std::size_t N, template <typename, std::size_t> typename ArrayLike>
struct to_array_like_fn
{
    template <ranges::input_range Rng>
        requires(ranges::sized_range<Rng>)
    constexpr auto operator()(Rng&& rng) const {
        using ContainedT = ranges::range_value_t<Rng>;

        using ContainerT = ArrayLike<ContainedT, N>;

        return to_static_container_fn<ContainerT>{}(std::forward<Rng>(rng));
    }
};

/// A pipeable adaptor to convert to any static container.
/// Just like `ranges::to`, but statically sized.
template <ranges::range StaticallySizedContainer>
    requires(HasStaticSize<StaticallySizedContainer>)
constexpr auto static_to() {
    return ranges::views::view_closure<to_static_container_fn<StaticallySizedContainer>>{};
};

/// \overload
/// Specify a static size and an "array-like" class template to convert to
template <std::size_t N, template <typename, std::size_t> typename ArrayLike>
constexpr auto static_to() {
    return ranges::views::view_closure<to_array_like_fn<N, ArrayLike>>{};
};

/// A pipeable adaptor to a std::array
template <std::size_t N>
constexpr auto to_array() {
    return static_to<N, std::array>();
};

namespace detail {

template <ranges::range Container>
struct maybe_static_to_impl;

template <ranges::range Container>
    requires(HasStaticSize<Container>)
struct maybe_static_to_impl<Container>
{
    using type = decltype(static_to<Container>());
};

template <ranges::range Container>
    requires(!HasStaticSize<Container>)
struct maybe_static_to_impl<Container>
{
    using type = decltype(ranges::to<Container>());
};

} // namespace detail

/// A pipeable adaptor to convert to any container, statically or dynamically sized.
/// Internally uses `static_to` for supported containers, and `ranges::to` for all others.
template <ranges::range Container>
constexpr auto maybe_static_to() {
    return typename detail::maybe_static_to_impl<Container>::type{};
}

} // namespace asmgrader
