#pragma once

#include "api/test_base.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace asmgrader {

/// Manages test execution and result aggregation for a specific assignment
class AssignmentTestRunner
{
public:
    AssignmentTestRunner(Assignment& assignment, const std::shared_ptr<Serializer>& serializer,
                         const std::optional<std::string>& tests_filter);

    AssignmentResult run_all(std::optional<std::filesystem::path> alternative_path) const;

private:
    TestResult run_one(TestBase& test) const;

    Assignment* assignment_;
    std::shared_ptr<Serializer> serializer_;
    std::optional<std::string> filter_;
};

} // namespace asmgrader
