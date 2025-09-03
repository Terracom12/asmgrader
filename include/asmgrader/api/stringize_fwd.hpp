#pragma once

#include <concepts>
#include <string>

/// For everything related to test requirement stringification
///
/// Contains implementations of the `stringize` function for all types that should be printed out
/// serialized in a special manner. Output will default to \ref format_or_unknown for all types
/// without a stringize overload.
namespace asmgrader::stringize {

/// No argument overload. Returns the empty string.
inline std::string stringize() {
    return "";
}

/// I love this name
template <typename Arg>
concept Stringizable = requires(Arg arg) {
    { stringize(arg) } -> std::same_as<std::string>;
};

} // namespace asmgrader::stringize
