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

template <typename Arg>
inline std::string try_stringize(Arg&& arg);

} // namespace asmgrader::stringize
