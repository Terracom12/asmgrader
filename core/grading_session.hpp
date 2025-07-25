#pragma once

#include <source_location>
#include <string>
#include <string_view>
#include <vector>

/// Defines data classes to store result data for the current run session

struct RequirementResult
{
    bool passed;
    std::string msg;

    struct DebugInfo
    {
        std::string_view msg;     // stringified condition
        std::source_location loc; // requirement execution point

        constexpr static std::string_view DEFAULT_MSG = "<unknown>";

        explicit DebugInfo(std::string_view message = DEFAULT_MSG,
                           std::source_location location = std::source_location::current())
            : msg{message}
            , loc{location} {}
    };

    DebugInfo debug_info;
};

struct TestResult
{
    std::string name;
    std::vector<RequirementResult> requirement_results;

    int num_passed{};
    int num_total{};

    constexpr bool passed() const noexcept { return num_failed() == 0; }
    constexpr int num_failed() const noexcept { return num_total - num_passed; }
};

struct AssignmentResult
{
    std::string name;
    std::vector<TestResult> test_results;
};

struct GradingSession
{
};
