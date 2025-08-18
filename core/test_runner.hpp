#pragma once

#include "api/test_base.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"

#include <filesystem>
#include <memory>
#include <optional>

/// Manages test execution and result aggregation for a specific assignment
class AssignmentTestRunner
{
public:
    AssignmentTestRunner(Assignment& assignment, std::unique_ptr<Serializer> serializer);

    AssignmentResult run_all(std::optional<std::filesystem::path> alternative_path) const;

private:
    TestResult run_one(TestBase& test) const;

    Assignment* assignment_;
    std::unique_ptr<Serializer> serializer_;
};
