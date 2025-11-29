#pragma once

#include <asmgrader/grading_session.hpp>

#include "app/app.hpp" // IWYU pragma: export

#include <optional>
#include <vector>

namespace asmgrader {

class ProfessorApp final : public App
{
public:
    using App::App;

private:
    int run_impl() override;

    std::optional<std::vector<StudentInfo>> get_student_names() const;
};

} // namespace asmgrader
