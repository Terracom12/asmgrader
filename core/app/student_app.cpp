#include "app/student_app.hpp"

#include "api/assignment.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "output/plaintext_serializer.hpp"
#include "output/stdout_sink.hpp"
#include "registrars/global_registrar.hpp"
#include "test_runner.hpp"
#include "user/program_options.hpp"

#include <fmt/format.h>

#include <memory>
#include <optional>
#include <utility>

int StudentApp::run_impl() {
    LOG_TRACE("Registered tests: {::}", GlobalRegistrar::get().for_each_assignment([](const Assignment& assignment) {
        return fmt::format("{:?}: {}", assignment.get_name(), assignment.get_test_names());
    }));

    auto assignment = GlobalRegistrar::get().get_assignment(OPTS.assignment_name);

    ASSERT(assignment, "Error locating assignment {}", OPTS.assignment_name);

    using enum ProgramOptions::VerbosityLevel;
    StdoutSink output_sink;
    auto output_serializer =
        std::make_unique<PlainTextSerializer>(output_sink, OPTS.colorize_option, OPTS.verbosity > Quiet);
    AssignmentTestRunner runner{*assignment, std::move(output_serializer)};

    AssignmentResult res = runner.run_all(OPTS.file_name);

    return res.num_tests_failed();
}
