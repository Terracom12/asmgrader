#include "grading_session.hpp"
#include "logging.hpp"
#include "output/plaintext_serializer.hpp"
#include "output/stdout_sink.hpp"
#include "registrars/global_registrar.hpp"
#include "test/assignment.hpp"
#include "test/test_base.hpp"
#include "test_runner.hpp"
#include "user/cl_args.hpp"
#include "user/program_options.hpp"

#include <boost/stacktrace/stacktrace.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <range/v3/algorithm/count.hpp>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <utility>

namespace {
template <typename T>
void handle_exception(const T& exception) {
    boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace::from_current_exception();
    std::string except_str = fmt::format("Unhandled exception caught in main: {}", exception);
    fmt::println(std::cerr, "{}", except_str);
    fmt::println(std::cerr, "{}", std::string(except_str.size(), '='));

    std::string stacktrace_str = fmt::to_string(fmt::streamed(trace));
    fmt::println(std::cerr, "Stacktrace:\n{}", stacktrace_str.empty() ? " <unavailable>" : stacktrace_str);
}
} // namespace

int main(int argc, const char* argv[]) {
    init_loggers();

    try {
        LOG_TRACE("Registered tests: {::}",
                  GlobalRegistrar::get().for_each_assignment([](const Assignment& assignment) {
                      return fmt::format("{:?}: {}", assignment.get_name(), assignment.get_test_names());
                  }));

        std::span<const char*> args{argv, static_cast<std::size_t>(argc)};

        const ProgramOptions options = parse_args_or_exit(args);

        auto assignment = GlobalRegistrar::get().get_assignment(options.assignment_name);

        ASSERT(assignment, "Error locating assignment {}", options.assignment_name);

        StdoutSink output_sink;
        auto output_serializer =
            std::make_unique<PlainTextSerializer>(output_sink, options.colorize_option, options.verbose);
        TestRunner runner{*assignment, std::move(output_serializer)};

        AssignmentResult res = runner.run_all(options.file_name);

        return res.num_tests_failed();
    } catch (const std::exception& ex) {
        handle_exception(ex);
    } catch (...) {
        handle_exception("(unknown - not derived from std::exception)");
    }
}
