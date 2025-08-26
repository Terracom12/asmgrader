#include "multi_student_runner.hpp"

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"
#include "test_runner.hpp"

#include <fmt/compile.h>

#include <memory>
#include <utility>
#include <vector>

namespace asmgrader {

MultiStudentRunner::MultiStudentRunner(Assignment& assignment, const std::shared_ptr<Serializer>& serializer)
    : assignment_{&assignment}
    , serializer_{serializer} {}

MultiStudentResult MultiStudentRunner::run_all_students(const std::vector<StudentInfo>& students) const {
    MultiStudentResult result;

    AssignmentTestRunner assignment_runner{*assignment_, serializer_};

    for (const StudentInfo& info : students) {
        serializer_->on_student_begin(info);

        AssignmentResult assignment_res;

        if (info.assignment_path.has_value()) {
            assignment_res = assignment_runner.run_all(info.assignment_path);
        }

        StudentResult res{.info = info, .result = assignment_res};

        serializer_->on_student_end(info);

        result.results.push_back(std::move(res));
    }

    return result;
}

} // namespace asmgrader
