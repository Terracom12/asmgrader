#pragma once

#include "grading_session.hpp"
#include "program/program.hpp"

#include <string>

/// User-facing API for use within an assignment test case for:
///   Interacting with or querying data for the student's assembled binary
///   Test result collection (requirements, pass/fail cases)
///   Performing any other esoteric test-related action
class TestContext
{
public:
    explicit TestContext(std::string name, Program program) noexcept
        : prog_{std::move(program)}
        , result_{.name = std::move(name), .requirement_results = {}, .num_passed = 0, .num_total = 0} {}

    void require(bool condition, std::string msg = "",
                 RequirementResult::DebugInfo debug_info = RequirementResult::DebugInfo{});

    /// Obtain the final test results
    /// Run after the test is complete. Note: has no ill effects if run before test is complete
    TestResult finalize();

private:
    Program prog_;

    /// Mutable result_. Not useful for output until ``finalize`` is called
    TestResult result_;
};
