#pragma once

#include "app/app.hpp" // IWYU pragma: export

namespace asmgrader {

class StudentApp final : public App
{
public:
    using App::App;

private:
    int run_impl() override;
};

} // namespace asmgrader
