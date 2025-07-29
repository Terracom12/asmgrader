#include "test_context.hpp"

#include "grading_session.hpp"
#include "program/program.hpp"
#include "test/test_base.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <range/v3/algorithm/count.hpp>

TestContext::TestContext(TestBase& test, Program program) noexcept
    : associated_test_{&test}
    , prog_{std::move(program)}
    , result_{.name = std::string{test.get_name()}, .requirement_results = {}, .num_passed = 0, .num_total = 0} {}

TestResult TestContext::finalize() {
    result_.num_passed = static_cast<int>(
        ranges::count(result_.requirement_results, true, [](const RequirementResult& res) { return res.passed; }));
    result_.num_total = static_cast<int>(result_.requirement_results.size());

    return result_;
}
bool TestContext::require(bool condition, RequirementResult::DebugInfo debug_info) {
    return require(condition, "<no message>", debug_info);
}
bool TestContext::require(bool condition, std::string msg, RequirementResult::DebugInfo debug_info) {
    std::string req_str;

    if (condition) {
        req_str = fmt::format("{}", fmt::styled("PASSED", fg(fmt::color::green)));
    } else {
        req_str = fmt::format("{}", fmt::styled("FAILED", fg(fmt::color::red)));
    }

    LOG_DEBUG("Requirement {}: {:?} ({})", std::move(req_str), fmt::styled(msg, fg(fmt::color::aqua)), debug_info);

    result_.requirement_results.emplace_back(/*.passed =*/condition, /*.msg =*/std::move(msg),
                                             /*.debug_info =*/debug_info);
    return condition;
}
std::string_view TestContext::get_name() const {
    return associated_test_->get_name();
}

util::Result<std::string> TestContext::get_stdout() {
    auto out = prog_.get_subproc().read_stdout();

    if (!out) {
        return util::ErrorKind::UnknownError;
    }

    return *out;
}
std::string TestContext::get_full_stdout() {
    return prog_.get_subproc().get_full_stdout();
}

util::Result<RunResult> TestContext::run() {
    return prog_.run();
}
