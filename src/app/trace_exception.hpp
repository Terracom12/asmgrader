#pragma once

#include "common/extra_formatters.hpp" // IWYU pragma: keep

#include <boost/stacktrace/stacktrace.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace asmgrader {

template <typename T>
void trace_exception(const T& exception) {
    boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace::from_current_exception();
    std::string except_str = fmt::format("Unhandled exception: {}", exception);
    fmt::println(std::cerr, "{}", except_str);
    fmt::println(std::cerr, "{}", std::string(except_str.size(), '='));

    std::string stacktrace_str = fmt::to_string(fmt::streamed(trace));
    fmt::println(std::cerr, "Stacktrace:\n{}", stacktrace_str.empty() ? " <unavailable>" : stacktrace_str);
}

template <typename Func, typename... Args>
    requires(std::invocable<Func, Args...>)
std::optional<std::invoke_result_t<Func, Args...>> wrap_throwable_fn(Func&& fn, Args&&... args) {
    try {
        return std::invoke(std::forward<Func>(fn), std::forward<Args>(args)...);
    } catch (const std::exception& ex) {
        trace_exception(ex);
    } catch (...) {
        trace_exception("<unknown - not derived from std::exception>");
    }

    return std::nullopt;
}

} // namespace asmgrader
