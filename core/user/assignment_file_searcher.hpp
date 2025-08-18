#pragma once

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "user/file_searcher.hpp"

#include <string>
#include <utility>

class AssignmentFileSearcher : public FileSearcher
{
public:
    static constexpr auto DEFAULT_REGEX = R"(`lastname``firstname`_`base`-\d+\.`ext`)";

    /// Unknown student name; search for any word in place of `lastname``firstname`
    explicit AssignmentFileSearcher(const Assignment& assignment,
                                    const StudentInfo& student_info = {.first_name = "\\w", .last_name = ""},
                                    std::string var_regex = DEFAULT_REGEX)
        : FileSearcher{std::move(var_regex)} {
        const auto base_split = assignment.get_exec_path().string().find_last_of('.');

        set_arg("base", var_regex.substr(0, base_split));
        set_arg("ext", var_regex.substr(base_split));
        set_arg("firstname", student_info.first_name);
        set_arg("lastname", student_info.last_name);
    }
};
