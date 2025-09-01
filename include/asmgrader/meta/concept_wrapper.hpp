#pragma once

namespace asmgrader {

template <typename T, template <typename> typename Template>
concept C = Template<T>::value;

} // namespace asmgrader
