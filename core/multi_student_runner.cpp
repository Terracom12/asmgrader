#include "multi_student_runner.hpp"

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "output/serializer.hpp"
#include "test_runner.hpp"

#include <memory>
#include <utility>
#include <vector>

MultiStudentRunner::MultiStudentRunner(Assignment& assignment, const std::shared_ptr<Serializer>& serializer)
    : assignment_{&assignment}
    , serializer_{serializer} {}

MultiStudentResult MultiStudentRunner::run_all_students(const std::vector<StudentInfo>& students) const {
    MultiStudentResult result;

    AssignmentTestRunner assignment_runner{*assignment_, serializer_};

    for (const StudentInfo& info : students) {
        // TODO: Serialize student info

        StudentResult res{.info = info, .result = assignment_runner.run_all(info.assignment_path)};

        result.results.push_back(std::move(res));
    }

    return result;
}
