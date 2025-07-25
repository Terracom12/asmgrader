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

    std::string_view get_name() const noexcept { return name_; }

    template <typename Func>
    AsmFunction<Func> find_function(std::string name);

    std::string get_stdout();

    virtual void run(TestContext& ctx) = 0;

    const Assignment& get_assignment() const { return *assignment_; }

private:
    std::string_view name_;
    const Assignment* assignment_;
};

template <typename Func>
AsmFunction<Func> TestBase::find_function(std::string name) {
    // UNIMPLEMENTED();
    std::ignore = name;
    return {std::move(name), 0x0};
}
