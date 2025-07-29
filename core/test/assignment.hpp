#pragma once

#include "test/test_base.hpp"
#include "user/cl_args.hpp"
#include "util/class_traits.hpp"

#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

/// Declaration for the logic and data encapsulating a class assignment
///
/// This includes executable path, all of the tests we want to run,
/// and any other necessary actions.
class Assignment : util::NonCopyable
{
public:
    Assignment(std::string_view name, std::filesystem::path exec_path) noexcept;

    void add_test(std::unique_ptr<TestBase> test) const noexcept;

    std::string_view get_name() const noexcept { return name_; }
    std::filesystem::path get_exec_path() const noexcept {
        // FIXME: This is a bit jank
        if (program_args->file_name) {
            return *program_args->file_name;
        }
        return exec_path_;
    }

    auto get_tests() const noexcept {
        return tests_ | ranges::views::transform([](std::unique_ptr<TestBase>& test) -> TestBase& { return *test; });
    }

    std::vector<std::string_view> get_test_names() const noexcept {
        return tests_ |
               ranges::views::transform([](const std::unique_ptr<TestBase>& test) { return test->get_name(); }) |
               ranges::to<std::vector>();
    }

private:
    std::string_view name_;
    std::filesystem::path exec_path_;

    mutable std::vector<std::unique_ptr<TestBase>> tests_;
};
