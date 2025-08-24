#include "api/assignment.hpp"

#include "api/test_base.hpp"

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

namespace asmgrader {

Assignment::Assignment(std::string_view name, std::filesystem::path exec_path) noexcept
    : name_{name}
    , exec_path_{std::move(exec_path)} {}

void Assignment::add_test(std::unique_ptr<TestBase> test) const noexcept {
    tests_.push_back(std::move(test));
}

} // namespace asmgrader
