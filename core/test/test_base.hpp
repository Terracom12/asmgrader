#pragma once

#include "test/asm_function.hpp"
#include "test_context.hpp"

#include <string>
#include <string_view>

class Assignment;

/// Base class primarily for a user-written test
///
/// Provides member functions for any test action the user might want to perform within a test case.
class TestBase
{
public:
    explicit TestBase(const Assignment& assignment, std::string_view name, int /*options*/ = 0) noexcept
        : name_{name}
        , assignment_{&assignment} {}
    virtual ~TestBase() noexcept = default;

    virtual void run(TestContext& ctx) = 0;

    const Assignment& get_assignment() const { return *assignment_; }

    std::string_view get_name() const noexcept { return name_; }

private:
    std::string_view name_;
    const Assignment* assignment_;
};
