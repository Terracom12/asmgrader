#include "catch2_custom.hpp"

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "user/assignment_file_searcher.hpp"
#include "user/file_searcher.hpp"

#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <range/v3/action/sort.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using Catch::Matchers::UnorderedRangeEquals;

const auto resources_path = std::filesystem::path{RESOURCES_DIR} / "file_searching";

const auto map_to_basename =
    ranges::views::transform([](const std::filesystem::path& path) { return path.stem().string(); }) |
    ranges::to<std::vector>();

const auto map_to_filename =
    ranges::views::transform([](const std::filesystem::path& path) { return path.filename().string(); }) |
    ranges::to<std::vector>();

const auto extract_path =
    ranges::views::transform([](const asmgrader::StudentInfo& info) { return info.assignment_path.value(); });

constexpr auto make_student = [](std::string first_name, std::string last_name) {
    return asmgrader::StudentInfo{.first_name = std::move(first_name),
                                  .last_name = std::move(last_name),
                                  .names_known = false,
                                  .assignment_path = {},
                                  .subst_regex_string = ""};
};

TEST_CASE("Find txt files") {
    asmgrader::FileSearcher searcher{".*\\.txt"};

    const std::vector<std::string> expected_first = {"a", "b", "c"};
    const std::vector<std::string> expected_second = [vec = expected_first]() mutable {
        vec.push_back("d");
        return vec;
    }();
    const std::vector<std::string> expected_third = [vec = expected_second]() mutable {
        vec.push_back("e");
        return vec;
    }();

    auto search_res_first = searcher.search(resources_path);
    REQUIRE_THAT(expected_first, UnorderedRangeEquals(search_res_first | map_to_basename));

    auto search_res_second = searcher.search_recursive(resources_path, 1);
    REQUIRE_THAT(expected_second, UnorderedRangeEquals(search_res_second | map_to_basename));

    auto search_res_third = searcher.search_recursive(resources_path, 2);
    REQUIRE_THAT(expected_third, UnorderedRangeEquals(search_res_third | map_to_basename));

    auto search_res_recursive = searcher.search_recursive(resources_path);
    REQUIRE_THAT(expected_third, UnorderedRangeEquals(search_res_recursive | map_to_basename));
}

TEST_CASE("Find txt files with variable substitution") {
    asmgrader::FileSearcher abc_searcher{"`name`\\.txt", {{"name", "[abc]"}}};
    asmgrader::FileSearcher de_searcher{"`name`\\.txt", {{"name", "[de]"}}};

    const std::vector<std::string> expected_abc = {"a", "b", "c"};
    const std::vector<std::string> expected_de = {"d", "e"};

    auto search_res_abc = abc_searcher.search_recursive(resources_path);
    REQUIRE_THAT(expected_abc, UnorderedRangeEquals(search_res_abc | map_to_basename));

    auto search_res_de = de_searcher.search_recursive(resources_path);
    REQUIRE_THAT(expected_de, UnorderedRangeEquals(search_res_de | map_to_basename));
}

// weird file name to test that an extra `.` won't mess up basename / extension extraction logic
const asmgrader::Assignment assignment{"", "exec.foo.out"};

TEST_CASE("Find assignment files with default search expression") {
    asmgrader::AssignmentFileSearcher searcher{assignment};

    const std::vector<std::string> expected = {"doejohn_0000_0000_exec.foo.out", "doejane_0000_0000_exec.foo.out"};
    const std::vector<std::string> expected_recursive = [vec = expected]() mutable {
        vec.emplace_back("liddellalice_0000_0000_exec.foo.out");
        vec.emplace_back("robertsbob_0000_0000_exec.foo.out");
        return vec;
    }();

    auto search_res = searcher.search(resources_path);
    REQUIRE_THAT(expected, UnorderedRangeEquals(search_res | extract_path | map_to_filename));

    auto search_res_recursive = searcher.search_recursive(resources_path);
    REQUIRE_THAT(expected_recursive, UnorderedRangeEquals(search_res_recursive | extract_path | map_to_filename));
}

TEST_CASE("Find assignment files with specified student names") {
    asmgrader::AssignmentFileSearcher searcher{assignment};

    const auto tests = std::to_array<std::pair<asmgrader::StudentInfo, std::string_view>>({
        {make_student("John", "Doe"), "doejohn_0000_0000_exec.foo.out"},
        {make_student("Jane", "Doe"), "doejane_0000_0000_exec.foo.out"},
        {make_student("Alice", "Liddell"), "liddellalice_0000_0000_exec.foo.out"},
        {make_student("Bob", "Roberts"), "robertsbob_0000_0000_exec.foo.out"},
    });

    for (auto [student, expected_filename] : tests) {
        auto search_res = searcher.search_recursive(student, resources_path);
        REQUIRE(search_res);
        REQUIRE(student.assignment_path->filename().c_str() == expected_filename);
    }

    auto student_false_test = make_student("Unknown", "Unknown");
    auto search_res = searcher.search_recursive(student_false_test, resources_path);
    REQUIRE_FALSE(search_res);
    REQUIRE(student_false_test.assignment_path->empty());
}

TEST_CASE("Find assignment files with special student names") {
    const asmgrader::Assignment special_assignment{"", "special.out"};
    asmgrader::AssignmentFileSearcher searcher{special_assignment};

    const auto tests = std::to_array<std::pair<asmgrader::StudentInfo, std::string_view>>({
        {make_student("Jack", "O'Reily"), "o'reilyjack_0000_0000_special.out"},
        {make_student("Carlos", "De La Cruz"), "de la cruzcarlos_0000_0000_special.out"},
    });

    for (auto [student, expected_filename] : tests) {
        auto search_res = searcher.search_recursive(student, resources_path);
        REQUIRE(search_res);
        REQUIRE(student.assignment_path->filename().c_str() == expected_filename);
    }
}
