#ifdef PROFESSOR_VERSION
#include "app/professor_app.hpp"
#else
#include "app/student_app.hpp"
#endif // PROFESSOR_VERSION
#include "app/test_theme_app.hpp"
#include "logging.hpp"
#include "user/cl_args.hpp"
#include "user/program_options.hpp"

#include <boost/stacktrace/stacktrace.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <range/v3/view/transform.hpp>

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <span>

int main(int argc, const char* argv[]) {
    asmgrader::init_loggers();

    std::span<const char*> args{argv, static_cast<std::size_t>(argc)};
    const asmgrader::ProgramOptions options = asmgrader::parse_args_or_exit(args);

    std::unique_ptr<asmgrader::App> app;

    if (options.test_syntax_highlighter) {
        app = std::make_unique<asmgrader::TestThemeApp>(options);
        return app->run();
    }

#ifdef PROFESSOR_VERSION
    app = std::make_unique<asmgrader::ProfessorApp>(options);
#else
    app = std::make_unique<asmgrader::StudentApp>(options);
#endif // PROFESSOR_VERSION

    return app->run();
}
