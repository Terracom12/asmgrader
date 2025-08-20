#pragma once

#include "common/error_types.hpp"
#include "common/extra_formatters.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <stdexcept>
#include <string>

/// Error for any internal failure conditions of TestContext
class ContextInternalError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;

    explicit ContextInternalError(util::ErrorKind error, const std::string& msg = "")
        : std::runtime_error{msg}
        , error_{error} {}

    util::ErrorKind get_error() const { return error_; };

private:
    util::ErrorKind error_ = util::ErrorKind::UnknownError;
};

template <>
struct fmt::formatter<ContextInternalError> : DebugFormatter
{
    auto format(const ContextInternalError& from, format_context& ctx) const {
        return format_to(ctx.out(), "{} : {}", from.what(), fmt::underlying(from.get_error()));
    }
};
