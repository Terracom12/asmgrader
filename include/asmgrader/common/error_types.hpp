#pragma once

#include <asmgrader/common/expected.hpp>
#include <asmgrader/common/formatters/macros.hpp>

#include <boost/preprocessor/cat.hpp>

namespace asmgrader {

// NOLINTNEXTLINE
enum class ErrorKind {
    TimedOut,         ///< Program / operation surpassed maximum timeout (gennerally specified)
    UnresolvedSymbol, ///< Failed to resolve a named symbol in a program
    UnexpectedReturn, ///< A function returned happened due to an unexpected condition
    BadArgument,      ///< Bad argument to an AsmFunction. For an unwrappable type with no inner value.
    UnknownError,     ///< As named; use this as little as possible
    SyscallFailure,   ///< A Linux syscall failed

    MaxErrorNum // Not a proper error; used to determine the number of errors
};

template <typename T>
using Result = Expected<T, ErrorKind>;

} // namespace asmgrader

FMT_SERIALIZE_ENUM(::asmgrader::ErrorKind, TimedOut, UnresolvedSymbol, UnexpectedReturn, BadArgument, UnknownError,
                   SyscallFailure, MaxErrorNum);

/// If the supplied argument is an error (unexpected) type, then propegate the error type `e` up
/// the call stack. Otherwise, continue execution as normal
// NOLINTBEGIN(bugprone-macro-parentheses)
#define TRYE_IMPL(val, e, ident)                                                                                       \
    __extension__({                                                                                                    \
        const auto& ident = val;                                                                                       \
        if (!ident.has_value()) {                                                                                      \
            using enum ErrorKind;                                                                                      \
            return e;                                                                                                  \
        }                                                                                                              \
        ident.value();                                                                                                 \
    })

#define TRY_IMPL(val, ident) TRYE_IMPL(val, ident.error(), ident)
// NOLINTEND(bugprone-macro-parentheses)

#define TRYE(val, e) TRYE_IMPL(val, e, BOOST_PP_CAT(errref_uniq__, __COUNTER__))

/// If the supplied argument is an error (unexpected) type, then propegate it up the call stack.
/// Otherwise, continue execution as normal
#define TRY(val) TRY_IMPL(val, BOOST_PP_CAT(errrefe_uniq__, __COUNTER__))
