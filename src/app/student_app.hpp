#pragma once

#include "api/assignment.hpp"
#include "app/app.hpp" // IWYU pragma: export

namespace asmgrader {

class StudentApp final : public App
{
public:
    using App::App;

private:
    int run_impl() override;

    Assignment& get_assignment_or_exit() const;
};

} // namespace asmgrader
