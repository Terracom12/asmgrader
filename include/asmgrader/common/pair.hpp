#pragma once

namespace asmgrader {

// Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97930 for gcc versions < 12.2
template <typename T, typename U>
struct pair // NOLINT(readability-identifier-naming)
{
    T first;
    U second;
};

} // namespace asmgrader
