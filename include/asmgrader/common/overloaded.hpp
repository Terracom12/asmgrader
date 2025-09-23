#pragma once

namespace asmgrader {

// https://en.cppreference.com/w/cpp/utility/variant/visit2.html
template <typename... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};

} // namespace asmgrader
