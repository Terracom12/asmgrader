#include "app/student_app.hpp"

#include "api/assignment.hpp"
#include "common/linux.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "output/plaintext_serializer.hpp"
#include "output/stdout_sink.hpp"
#include "output/verbosity.hpp"
#include "registrars/global_registrar.hpp"
#include "test_runner.hpp"
#include "user/program_options.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <range/v3/view/transform.hpp>

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

namespace asmgrader {

namespace {

bool can_execute(const std::filesystem::path& path) {
    namespace fs = std::filesystem;

    if (!fs::is_regular_file(path)) {
        LOG_TRACE("{} is not a regular file", path);
        return false;
    }

    if (!linux::access(path.c_str(), X_OK)) {
        LOG_TRACE("{} is not executable by this program", path);
        return false;
    }

    return true;
}

std::reference_wrapper<Assignment> infer_assignment_or_exit() {
    namespace fs = std::filesystem;

    auto& registrar = GlobalRegistrar::get();

    std::vector<std::reference_wrapper<Assignment>> infer_res;

    for (fs::path cwd = fs::current_path(); const auto& dirent : fs::directory_iterator{cwd}) {
        const auto& path = dirent.path();
        std::string pathname = path.filename().string();
        LOG_TRACE("Checking if {:?} matches any assignments {}", pathname, dirent.path());

        if (!can_execute(path)) {
            continue;
        }

        if (auto found = registrar.get_assignment_by_pathname(pathname)) {
            LOG_TRACE("{:?} matches an assignment", pathname);
            infer_res.push_back(found.value());
        }
    }

    if (infer_res.size() == 0) {
        fmt::println(stderr, "Could not infer assignment based on files in the current working directory! Please "
                             "either change directories or specify the assignment explicitly by name. Exiting.");
        std::exit(1);
    }

    if (infer_res.size() > 1) {
        fmt::println(
            stderr,
            "More than 1 valid assignment found in current working directory when attempting to infer "
            "assignment.\nThis is ambiguous! Please specify the assignment explicitly by name.\n(Found: {::?})",
            infer_res |
                ranges::views::transform([](auto& assignment_ref) { return assignment_ref.get().get_exec_path(); }));
        std::exit(1);
    }

    // infer_res.size() == 1
    return infer_res.front();
}

} // namespace

Assignment& StudentApp::get_assignment_or_exit() const {
    if (OPTS.assignment_name.empty()) {
        LOG_DEBUG("Assignment unspecified; attempting to infer based on files in cwd.");
        return infer_assignment_or_exit();
    }

    auto opt_assignment = GlobalRegistrar::get().get_assignment(OPTS.assignment_name);
    ASSERT(opt_assignment.has_value(), "Internal error locating assignment", OPTS.assignment_name);

    return opt_assignment.value();
}

int StudentApp::run_impl() {
    LOG_TRACE("Registered tests: {::}", GlobalRegistrar::get().for_each_assignment([](const Assignment& assignment) {
        return fmt::format("{:?}: {}", assignment.get_name(), assignment.get_test_names());
    }));

    Assignment& assignment = get_assignment_or_exit();

    StdoutSink output_sink;
    std::shared_ptr output_serializer =
        std::make_shared<PlainTextSerializer>(output_sink, OPTS.colorize_option, OPTS.verbosity);
    AssignmentTestRunner runner{assignment, output_serializer, OPTS.tests_filter};

    output_serializer->on_run_metadata(RunMetadata{});
    AssignmentResult res = runner.run_all(OPTS.file_name);

    if (OPTS.verbosity == VerbosityLevel::Silent) {
        return res.num_tests_failed();
    }

    return EXIT_SUCCESS;
}

} // namespace asmgrader
