#pragma once

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace asmgrader {

class MultiStudentRunner
{
public:
    MultiStudentRunner(Assignment& assignment, const std::shared_ptr<Serializer>& serializer,
                       const std::optional<std::string>& tests_filter);

    MultiStudentResult run_all_students(const std::vector<StudentInfo>& students) const;

private:
    Assignment* assignment_;
    std::shared_ptr<Serializer> serializer_;
    std::optional<std::string> filter_;
};

} // namespace asmgrader
