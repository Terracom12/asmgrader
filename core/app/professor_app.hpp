#pragma once

#include "app/app.hpp" // IWYU pragma: export
#include "grading_session.hpp"

#include <optional>
#include <vector>

class ProfessorApp final : public App
{
public:
    using App::App;

private:
    int run_impl() override;

    std::optional<std::vector<StudentInfo>> get_student_names() const;
};
