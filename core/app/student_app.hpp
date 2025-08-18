#pragma once

#include "app/app.hpp" // IWYU pragma: export

class StudentApp final : public App
{
public:
    using App::App;

private:
    int run_impl() override;
};
