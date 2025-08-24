#include "user/assignment_file_searcher.hpp"

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "user/file_searcher.hpp"

#include <range/v3/action/take_while.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace asmgrader {

AssignmentFileSearcher::AssignmentFileSearcher(const Assignment& assignment, std::string var_regex)
    : FileSearcher{std::move(var_regex)} {
    const std::string assignment_path = assignment.get_exec_path().string();
    const auto base_split = assignment_path.find_last_of('.');

    set_arg("base", assignment_path.substr(0, base_split));
    set_arg("ext", assignment_path.substr(base_split + 1));
}

bool AssignmentFileSearcher::search_recursive(StudentInfo& student, const std::filesystem::path& base, int max_depth) {
    namespace fs = std::filesystem;

    auto tolower = ranges::views::transform(static_cast<int (*)(int)>(std::tolower)) | ranges::to<std::string>();

    set_arg("firstname", student.first_name | tolower);
    set_arg("lastname", student.last_name | tolower);

    std::vector matching_files = FileSearcher::search_recursive(base, max_depth);

    if (matching_files.size() == 0) {
        return false;
    }

    if (matching_files.size() > 1) {
        ranges::sort(matching_files, std::less<>{}, [](const fs::path& path) { return fs::last_write_time(path); });

        LOG_WARN("Multiple files found for student {}: {}. Choosing the most recent one ({})", student,
                 matching_files | ranges::views::transform(std::mem_fn(&fs::path::c_str)),
                 matching_files.back().string());
    }

    // Choose the file most recently written to
    student.assignment_path = matching_files.back();

    return true;
}

std::vector<StudentInfo> AssignmentFileSearcher::search_recursive(const std::filesystem::path& base, int max_depth) {
    set_arg("firstname", "\\w+");
    set_arg("lastname", "");

    std::vector search_res = FileSearcher::search_recursive(base, max_depth);

    return search_res                                                //
           | ranges::views::transform(infer_student_names_from_file) //
           | ranges::to<std::vector>();
}

StudentInfo AssignmentFileSearcher::infer_student_names_from_file(const std::filesystem::path& path) {
    // Name = leading alphabetic characters in file path
    std::string name_field =
        path.filename().string() | ranges::actions::take_while(static_cast<int (*)(int)>(std::isalpha));

    return {.first_name = name_field, .last_name = "", .names_known = false, .assignment_path = path};
}

} // namespace asmgrader
