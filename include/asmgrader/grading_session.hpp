/// Defines data classes to store result data for the current run session
#pragma once

// TODO: Rename this file

#include <asmgrader/common/extra_formatters.hpp>
#include <asmgrader/exceptions.hpp>
#include <asmgrader/version.hpp>

#include <boost/preprocessor/stringize.hpp>
#include <fmt/base.h>
#include <gsl/util>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/view/transform.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace asmgrader {

struct RunMetadata
{
    int version = get_version();
    std::string_view version_string = ASMGRADER_VERSION_STRING;
    std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
    std::string_view git_hash = BOOST_PP_STRINGIZE(ASMGRADER_VERSION_GIT_HASH);
};

struct RequirementResult
{
    bool passed;
    std::string msg;

    struct DebugInfo
    {
        std::string_view msg;     // stringified condition
        std::source_location loc; // requirement execution point

        static constexpr std::string_view DEFAULT_MSG = "<unknown>";

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
    std::string first_name; // if names_known = false, this holds the inferred name for now
    std::string last_name;
    bool names_known; // whether the names are known (i.e., obtained from a database) or inferred based on filename

    std::optional<std::filesystem::path> assignment_path;

    // the regex matcher, with fields substituted; used to provide a more helpful diagnostic
    std::string subst_regex_string;

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

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::RequirementResult::DebugInfo> : ::asmgrader::DebugFormatter
{
    constexpr auto format(const ::asmgrader::RequirementResult::DebugInfo& from, fmt::format_context& ctx) const {
        // Explicitly called "DebugInfo", so fmt to an empty string if not debug mode
        if (!is_debug_format) {
            return ctx.out();
        }

        return fmt::format_to(ctx.out(), "{{{} at {}}}", from.msg, from.loc);
    }
};
