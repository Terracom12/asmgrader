#pragma once

#include "grading_session.hpp"
#include "test/test_base.hpp"

#include <range/v3/range/concepts.hpp>
#include <range/v3/range/traits.hpp>

#include <concepts>
#include <iterator>
#include <utility>
#include <vector>

/// Manages test execution and result aggregation for a specific assignment
class TestRunner
{
public:
    explicit TestRunner(const Assignment& assignment);

    AssignmentResult run_all() const;

private:
    TestResult run_one(TestBase& test) const;

    const Assignment* assignment_;
};
