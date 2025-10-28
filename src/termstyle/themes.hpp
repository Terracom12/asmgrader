/// \file
/// Defines different stylistsic "classes" for colorization, and built-in themes.
#pragma once

#include "api/expression_inspection.hpp"
#include "termstyle/style.hpp"

#include <fmt/format.h>

#include <array>

namespace asmgrader::termstyle {

enum class StyleClass {
    VerboseInfo, ///<
    Info,        ///<
    Warning,     ///<
    Error,       ///<

    Max ///<
};

class Theme
{
public:
    constexpr Theme() = default;

private:
    std::array<Style, fmt::underlying(StyleClass::Max)> generic_styles_;
    std::array<Style, fmt::underlying(inspection::Token::Kind::Max)> cpp_syntax_styles_;
};

} // namespace asmgrader::termstyle
