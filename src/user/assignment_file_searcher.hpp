#pragma once

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "user/file_searcher.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace asmgrader {

class AssignmentFileSearcher : protected FileSearcher
{
public:
    static constexpr auto DEFAULT_REGEX = R"(`lastname``firstname`_`base`(-\d+)?\.`ext`)";
    static constexpr auto DEFAULT_SEARCH_DEPTH = 10;

    /// Unknown student name; search for any word in place of `lastname``firstname`
    explicit AssignmentFileSearcher(const Assignment& assignment, std::string var_regex = DEFAULT_REGEX);

    bool search_recursive(StudentInfo& student, const std::filesystem::path& base,
                          int max_depth = DEFAULT_SEARCH_DEPTH);

    /// Search for all potential student submissions for the given assignment
    std::vector<StudentInfo> search_recursive(const std::filesystem::path& base, int max_depth = DEFAULT_SEARCH_DEPTH);

private:
    static StudentInfo infer_student_names_from_file(const std::filesystem::path& path);
};

} // namespace asmgrader
