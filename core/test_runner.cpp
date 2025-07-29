#include "test_runner.hpp"

#include "grading_session.hpp"
#include "logging.hpp"
#include "test/assignment.hpp"

#include <ranges>
#include <unordered_map>

TestRunner::TestRunner(const Assignment& assignment)
    : assignment_{&assignment} {}

AssignmentResult TestRunner::run_all() const {
    // Assignment name -> TestResults
    std::unordered_map<std::string_view, std::vector<TestResult>> result;

    for (TestBase& test : assignment_->get_tests()) {
        const std::string_view assignment_name = test.get_assignment().get_name();

        const TestResult test_result = run_one(test);

        result[assignment_name].push_back(test_result);
    }

    if (result.size() > 1) {
        UNIMPLEMENTED("Multi-assignment runs are not yet supported! {}", result | std::ranges::views::keys);
    }

    return AssignmentResult{.name = std::string{result.begin()->first},
                            .test_results = std::move(result.begin()->second)};
}

TestResult TestRunner::run_one(TestBase& test) const {
    TestContext context(test, Program{assignment_->get_exec_path(), {}});

    test.run(context);

    return context.finalize();
}
