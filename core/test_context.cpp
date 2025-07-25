#include "test_context.hpp"

#include "grading_session.hpp"

#include <range/v3/algorithm/count.hpp>

TestResult TestContext::finalize() {
    result_.num_passed = static_cast<int>(
        ranges::count(result_.requirement_results, true, [](const RequirementResult& res) { return res.passed; }));
    result_.num_total = static_cast<int>(result_.requirement_results.size());

    return result_;
}
