#pragma once

#include "common/expected.hpp"

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

#include "common/macros.hpp"

/// If the supplied argument is an error (unexpected) type, then propegate the error type `e` up
/// the call stack. Otherwise, continue execution as normal
// NOLINTBEGIN(bugprone-macro-parentheses)
#define TRYE_IMPL(val, e, ident)                                                                                       \
    __extension__({                                                                                                    \
        const auto& ident = val;                                                                                       \
        if (ident.has_error()) {                                                                                       \
            using enum util::ErrorKind;                                                                                \
            return e;                                                                                                  \
        }                                                                                                              \
        ident.value();                                                                                                 \
    })

#define TRY_IMPL(val, ident) TRYE_IMPL(val, ident.error(), ident)
// NOLINTEND(bugprone-macro-parentheses)

#define TRYE(val, e) TRYE_IMPL(val, e, CONCAT(errref_uniq__, __COUNTER__))

/// If the supplied argument is an error (unexpected) type, then propegate it up the call stack.
/// Otherwise, continue execution as normal
#define TRY(val) TRY_IMPL(val, CONCAT(errrefe_uniq__, __COUNTER__))
