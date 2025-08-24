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

#include <cstdlib>
#include <memory>
#include <optional>

int StudentApp::run_impl() {
    LOG_TRACE("Registered tests: {::}", GlobalRegistrar::get().for_each_assignment([](const Assignment& assignment) {
        return fmt::format("{:?}: {}", assignment.get_name(), assignment.get_test_names());
    }));

    auto assignment = GlobalRegistrar::get().get_assignment(OPTS.assignment_name);

    ASSERT(assignment, "Error locating assignment {}", OPTS.assignment_name);

    using enum ProgramOptions::VerbosityLevel;

    StdoutSink output_sink;
    std::shared_ptr output_serializer =
        std::make_shared<PlainTextSerializer>(output_sink, OPTS.colorize_option, OPTS.verbosity);
    AssignmentTestRunner runner{*assignment, output_serializer};

    AssignmentResult res = runner.run_all(OPTS.file_name);

    if (OPTS.verbosity == ProgramOptions::VerbosityLevel::Silent) {
        return res.num_tests_failed();
    }

    return EXIT_SUCCESS;
}
