#include "test_runner.hpp"

#include "api/assignment.hpp"
#include "api/test_base.hpp"
#include "api/test_context.hpp"
#include "exceptions.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "output/serializer.hpp"
#include "program/program.hpp"

#include <range/v3/view/map.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

AssignmentTestRunner::AssignmentTestRunner(Assignment& assignment, std::unique_ptr<Serializer> serializer)
    : assignment_{&assignment}
    , serializer_{std::move(serializer)} {}

AssignmentResult AssignmentTestRunner::run_all(std::optional<std::filesystem::path> alternative_path) const {
    // Assignment name -> TestResults
    std::unordered_map<std::string_view, std::vector<TestResult>> result;
    int num_total_requirements = 0;

    if (alternative_path) {
        assignment_->set_exec_path(std::move(*alternative_path));
    }

    for (TestBase& test : assignment_->get_tests()) {
        const std::string_view assignment_name = test.get_assignment().get_name();

        const TestResult test_result = run_one(test);

        serializer_->on_test_result(test_result);

        result[assignment_name].push_back(test_result);

        num_total_requirements += test_result.num_total;
    }

    if (result.size() > 1) {
        UNIMPLEMENTED("Multi-assignment runs are not yet supported! {}", result | ranges::views::keys);
    }

    AssignmentResult res{.name = std::string{result.begin()->first},
                         .test_results = std::move(result.begin()->second),
                         .num_requirements_total = num_total_requirements};

    serializer_->on_assignment_result(res);

    return res;
}

TestResult AssignmentTestRunner::run_one(TestBase& test) const {
    TestContext context(test, Program{assignment_->get_exec_path(), {}});

    try {
        test.run(context);
    } catch (const ContextInternalError& ex) {
        LOG_DEBUG("Internal context test error: {}", ex);
        auto res = context.finalize();
        res.error = ex;
        return res;
    }

    return context.finalize();
}
