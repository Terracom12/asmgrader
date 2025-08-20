#pragma once

#include "exceptions.hpp" // IWYU pragma: export

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define STRINGIFY_IMPL(a) #a
#define STRINGIFY(a) STRINGIFY_IMPL(a)

#define TRY_OR_THROW(expr, ...)                                                                                        \
    __extension__({                                                                                                    \
        const auto& res__ref = (expr);                                                                                 \
        if (!res__ref) {                                                                                               \
            throw ContextInternalError{res__ref.error() __VA_OPT__(, ) __VA_ARGS__};                                   \
        }                                                                                                              \
        res__ref.value();                                                                                              \
    });
