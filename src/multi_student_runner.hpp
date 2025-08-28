#pragma once

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"

#include <memory>
#include <vector>

namespace asmgrader {

class MultiStudentRunner
{
public:
    MultiStudentRunner(Assignment& assignment, const std::shared_ptr<Serializer>& serializer);

    MultiStudentResult run_all_students(const std::vector<StudentInfo>& students) const;

private:
    Assignment* assignment_;
    std::shared_ptr<Serializer> serializer_;
};

} // namespace asmgrader
