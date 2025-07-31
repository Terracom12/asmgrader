#pragma once

#include "grading_session.hpp"
#include "program/program.hpp"
#include "subprocess/memory/concepts.hpp"
#include "test/asm_buffer.hpp"
#include "test/asm_data.hpp"
#include "test/asm_function.hpp"
#include "test/asm_symbol.hpp"
#include "util/byte_array.hpp"
#include "util/error_types.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>

class TestBase;

/// User-facing API for use within an assignment test case for:
///   Interacting with or querying data for the student's assembled binary
///   Test result collection (requirements, pass/fail cases)
///   Performing any other esoteric test-related action
class TestContext
{
public:
    explicit TestContext(TestBase& test, Program program) noexcept;

    bool require(bool condition, std::string msg,
                 RequirementResult::DebugInfo debug_info = RequirementResult::DebugInfo{});
    bool require(bool condition, RequirementResult::DebugInfo debug_info = RequirementResult::DebugInfo{});

    /// Obtain the final test results
    /// Run after the test is complete. Note: has no ill effects if run before test is complete
    TestResult finalize();

    ///// User-facing API

    /// Test name getter
    std::string_view get_name() const;

    /// Get any **new** stdout from the program since the last call to this function
    util::Result<std::string> get_stdout();

    /// Get all stdout from since the beginning of the test invokation
    std::string get_full_stdout();

    /// Find a named symbol in the associated program
    /// \tparam T  type of data that the symbol refers to
    template <typename T>
    AsmSymbol<T> find_symbol(std::string_view name);

    /// Create a buffer of ``NumBytes``
    template <std::size_t NumBytes>
    AsmBuffer<NumBytes> create_buffer();

    /// Find a named function in the associated program
    template <typename Func>
    AsmFunction<Func> find_function(std::string name);

    /// Run the program normally from `_start`
    util::Result<RunResult> run();

private:
    TestBase* associated_test_;
    Program prog_;

    /// Mutable result_. Not useful for output until ``finalize`` is called
    TestResult result_;
};

template <std::size_t NumBytes>
AsmBuffer<NumBytes> TestContext::create_buffer() {
    AsmBuffer<NumBytes> buffer{prog_};

    return buffer;
}

template <typename T>
AsmSymbol<T> TestContext::find_symbol(std::string_view name) {
    static_assert(MemoryReadSupported<T>, "Specified type does not have supported memory read operation! See docs.");

    auto symbol = prog_.get_symtab().find(name);

    if (!symbol) {
        LOG_DEBUG("Could not resolve symbol {:?}", name);
        return {prog_, std::string{name}, util::ErrorKind::UnresolvedSymbol};
    }

    return {prog_, std::string{name}, symbol->address};
}

template <typename Func>
AsmFunction<Func> TestContext::find_function(std::string name) {
    auto loc = prog_.get_symtab().find(name);

    if (!loc) {
        return {prog_, std::move(name), util::ErrorKind::UnresolvedSymbol};
    }

    return {prog_, std::move(name), loc->address};
}
