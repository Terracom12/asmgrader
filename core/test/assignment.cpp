#include "test/assignment.hpp"

#include "test/test_base.hpp"

Assignment::Assignment(std::string_view name, std::filesystem::path exec_path) noexcept
    : name_{name}
    , exec_path_{std::move(exec_path)} {}

void Assignment::add_test(std::unique_ptr<TestBase> test) const noexcept {
    tests_.push_back(std::move(test));
}
