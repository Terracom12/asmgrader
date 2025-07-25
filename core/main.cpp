#include "registrars/global_registrar.hpp"
#include "test/assignment.hpp"
#include "test/test_base.hpp"
#include "test_runner.hpp"
#include "user/cl_args.hpp"

#include <fmt/format.h>

#include <cstddef>
#include <cstdlib>
#include <span>

int main(int argc, const char* argv[]) {
    init_loggers();

    LOG_TRACE("Registered tests: {::}", GlobalRegistrar::get().for_each_assignment([](const Assignment& assignment) {
        return fmt::format("{:?}: {}", assignment.get_name(), assignment.get_test_names());
    }));

    std::span<const char*> args{argv, static_cast<std::size_t>(argc)};

    CommandLineArgs cl_args{args};

    if (auto parse_res = cl_args.parse(); parse_res) {
        cl_args.set_global();
    } else {
        fmt::println("{}\n{}", styled(parse_res.error(), fg(fmt::color::red)), cl_args.help_message());
        return EXIT_FAILURE;
    }

    auto assignment = GlobalRegistrar::get().get_assignment(program_args->assignment_name);

    ASSERT(assignment, "Error locating assignment {}", program_args->assignment_name);

    TestRunner runner{*assignment};

    auto res = runner.run_all();

    return EXIT_SUCCESS;
}
