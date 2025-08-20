#pragma once

// TODO: Rename this file

#include "exceptions.hpp"
#include "util/extra_formatters.hpp"

#include <fmt/base.h>
#include <gsl/util>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/view/transform.hpp>

#include <filesystem>
#include <functional>
#include <optional>
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

template <>
struct fmt::formatter<RequirementResult::DebugInfo> : DebugFormatter
{
    constexpr auto format(const RequirementResult::DebugInfo& from, fmt::format_context& ctx) const {
        // Explicitly called "DebugInfo", so fmt to an empty string if not debug mode
        if (is_debug_format) {
            return ctx.out();
        }

        return fmt::format_to(ctx.out(), "{{{} at {}}}", from.msg, from.loc);
    }
};

struct TestResult
{
    std::string name;
    std::vector<RequirementResult> requirement_results;

    int num_passed{};
    int num_total{};

    std::optional<ContextInternalError> error;

    constexpr bool passed() const noexcept { return !error && num_failed() == 0; }

    constexpr int num_failed() const noexcept { return num_total - num_passed; }
};

struct AssignmentResult
{
    std::string name;
    std::vector<TestResult> test_results;
    int num_requirements_total{};

    bool all_passed() const noexcept { return ranges::all_of(test_results, &TestResult::passed); }

    int num_tests_passed() const noexcept {
        return gsl::narrow_cast<int>(ranges::count_if(test_results, &TestResult::passed));
    }

    int num_tests_failed() const noexcept { return gsl::narrow_cast<int>(test_results.size()) - num_tests_passed(); }

    int num_requirements_failed() const noexcept {
        auto failed_view = test_results | ranges::views::transform(&TestResult::num_failed);
        return ranges::fold_left(failed_view, 0, std::plus<>{});
    }

    int num_requirements_passed() const noexcept { return num_requirements_total - num_requirements_failed(); }
};

// PROFESSOR_VERSION only

struct StudentInfo
{
    std::string first_name;
    std::string last_name;
    bool names_known; // whether the names are known (i.e., obtained from a database) or inferred based on filename

    std::optional<std::filesystem::path> assignment_path;

    // TODO: ids for students with the same names?
};

struct StudentResult
{
    StudentInfo info;
    AssignmentResult result;
};

struct MultiStudentResult
{
    std::vector<StudentResult> results;
};
