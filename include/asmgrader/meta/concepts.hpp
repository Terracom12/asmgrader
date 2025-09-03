#pragma once

#include <concepts>
#include <type_traits>

namespace asmgrader {

template <typename T, template <typename> typename Template>
concept C = Template<T>::value;

template <typename T, template <typename...> typename Template>
concept IsTemplate = requires(T value) {
    // only works with CTAD
    { Template{value} } -> std::convertible_to<std::decay_t<T>>;
};

} // namespace asmgrader
