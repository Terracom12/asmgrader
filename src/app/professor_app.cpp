#include "app/professor_app.hpp"

#include "app/student_app.hpp"
#include "common/expected.hpp"
#include "common/extra_formatters.hpp" // IWYU pragma: keep
#include "database_reader.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "multi_student_runner.hpp"
#include "output/plaintext_serializer.hpp"
#include "output/stdout_sink.hpp"
#include "registrars/global_registrar.hpp"
#include "user/assignment_file_searcher.hpp"
#include "user/program_options.hpp"

#include <gsl/util>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/for_each.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace asmgrader {

int ProfessorApp::run_impl() {
    std::optional student_names = get_student_names();
    LOG_DEBUG("Loaded student names: {}", student_names);

    auto assignment = GlobalRegistrar::get().get_assignment(OPTS.assignment_name);
    ASSERT(assignment, "Error locating assignment {}", OPTS.assignment_name);

    if (OPTS.file_name.has_value()) {
        return StudentApp{OPTS}.run();
    }

    AssignmentFileSearcher file_searcher{*assignment, OPTS.file_matcher};
    std::vector<StudentInfo> students;

    if (student_names.has_value()) {
        // TODO: Optimize this by just searching once and filtering files if they match
        //       MUCH more efficient than all the context switches that this will do...

        auto find_assignment_path = [this, &file_searcher](StudentInfo info) {
            file_searcher.search_recursive(info, OPTS.search_path);

            return info;
        };

        students = *student_names                                   //
                   | ranges::views::transform(find_assignment_path) //
                   | ranges::to<std::vector>();

        LOG_DEBUG("Found students assignments: {}", students);
    } else {
        students = file_searcher.search_recursive(OPTS.search_path);

        LOG_DEBUG("No database loaded. Inferred students: {}", students);
    }

    using enum ProgramOptions::VerbosityLevel;

    StdoutSink output_sink;
    std::shared_ptr output_serializer =
        std::make_shared<PlainTextSerializer>(output_sink, OPTS.colorize_option, OPTS.verbosity);

    MultiStudentRunner runner{*assignment, output_serializer};

    MultiStudentResult res = runner.run_all_students(students);

    auto num_students_failed =
        ranges::count_if(res.results, [](const StudentResult& sres) { return !sres.result.all_passed(); });

    if (OPTS.verbosity == ProgramOptions::VerbosityLevel::Silent) {
        return gsl::narrow_cast<int>(num_students_failed);
    }

    return EXIT_SUCCESS;
}

std::optional<std::vector<StudentInfo>> ProfessorApp::get_student_names() const {
    if (not OPTS.database_path) {
        return std::nullopt;
    }

    DatabaseReader database_reader{*OPTS.database_path};

    Expected res = database_reader.read();

    if (not res) {
        LOG_WARN("Error loading database {:?}: {}", OPTS.database_path->string(), res.error());
        return std::nullopt;
    }

    return *res;
}

} // namespace asmgrader
