#pragma once

#include "util/expected.hpp"

#include <boost/describe.hpp>

namespace util {

// NOLINTNEXTLINE
BOOST_DEFINE_ENUM_CLASS(ErrorKind,
                        TimedOut,         // Program / operation surpassed maximum timeout (gennerally specified)
                        UnresolvedSymbol, // Failed to resolve a named symbol in a program
                        UnexpectedReturn, // A function returned happened due to an unexpected condition
                        UnknownError,     // As named; use this as little as possible
                        SyscallFailure,   // A Linux syscall failed

                        MaxErrorNum // Not a proper error; used to determine the number of errors
);

template <typename T>
using Result = util::Expected<T, ErrorKind>;

} // namespace util

/// If the supplied argument is an error (unexpected) type, then propegate the error type `e` up
/// the call stack. Otherwise, continue execution as normal
//_Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define TRYE(val, e)                                                                                                   \
    __extension__({                                                                                                    \
        const auto& errref = val;                                                                                      \
        if (errref.has_error()) {                                                                                      \
            using enum util::ErrorKind;                                                                                \
            return e;                                                                                                  \
        }                                                                                                              \
        errref.value();                                                                                                \
    }) // _Pragma("GCC diagnostic pop")

/// If the supplied argument is an error (unexpected) type, then propegate it up the call stack.
/// Otherwise, continue execution as normal
#define TRY(val) TRYE(val, errref.error())
