#pragma once

#include "common/error_types.hpp"
#include "common/extra_formatters.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <stdexcept>
#include <string>

namespace asmgrader {

/// Error for any internal failure conditions of TestContext
class ContextInternalError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;

    explicit ContextInternalError(ErrorKind error, const std::string& msg = "")
        : std::runtime_error{msg}
        , error_{error} {}

    ErrorKind get_error() const { return error_; };

private:
    ErrorKind error_ = ErrorKind::UnknownError;
};

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::ContextInternalError> : ::asmgrader::DebugFormatter
{
    auto format(const ::asmgrader::ContextInternalError& from, format_context& ctx) const {
        return format_to(ctx.out(), "{} : {}", from.what(), fmt::underlying(from.get_error()));
    }
};
