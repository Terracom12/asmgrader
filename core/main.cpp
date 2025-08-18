#ifdef PROFESSOR_VERSION
#include "app/professor_app.hpp"
#else
#include "app/student_app.hpp"
#endif // PROFESSOR_VERSION

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
    init_loggers();

    std::span<const char*> args{argv, static_cast<std::size_t>(argc)};
    const ProgramOptions options = parse_args_or_exit(args);

    std::unique_ptr<App> app =
#ifdef PROFESSOR_VERSION
        std::make_unique<ProfessorApp>(options);
#else
        std::make_unique<StudentApp>(options);
#endif // PROFESSOR_VERSION

    return app->run();
}
