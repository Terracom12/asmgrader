#include "cl_args.hpp"

#include "api/assignment.hpp"
#include "registrars/global_registrar.hpp"
#include "user/program_options.hpp"
#include "util/expected.hpp"
#include "version.hpp"

#include <argparse/argparse.hpp>
#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

CommandLineArgs::CommandLineArgs(std::span<const char*> args)
    : arg_parser_{get_basename(args[0]), /*unused*/ ASMGRADER_VERSION_STRING, argparse::default_arguments::help}
    , args_{args.begin(), args.end()} {
    // Add parser arguments
    setup_parser();
}

void CommandLineArgs::setup_parser() {
    const auto assignment_names =
        GlobalRegistrar::get().for_each_assignment([&](const Assignment& assignment) { return assignment.get_name(); });

    constexpr auto VERSION_FMT = "AsmGrader v{}"
#ifdef PROFESSOR_VERSION
                                 " (Professor's Version)"
#endif
        ;
    arg_parser_.add_description(fmt::format(VERSION_FMT, ASMGRADER_VERSION_STRING));

    // clang-format off
    auto& assignment_arg = arg_parser_.add_argument("assignment")
        .store_into(opts_buffer_.assignment_name)
        // Add all assignment names to help msg, since argparse won't add choices by
        // default for some reason
        .help(fmt::format("The assignment to run tests on\nOne of: {:n}", assignment_names));
    GlobalRegistrar::get().for_each_assignment([&](const Assignment& assignment) {
        assignment_arg.add_choice(assignment.get_name());
    });

    // Verbatim from argparse.hpp, except replacing `-v` with `-V`
    arg_parser_.add_argument("-V", "--version")
        .action([&](const auto & /*unused*/) {
            fmt::println(ASMGRADER_VERSION_STRING);
            std::exit(0);
        })
        .default_value(false)
        .help("prints version information and exits")
        .implicit_value(true)
        .nargs(0);

    arg_parser_.add_argument("-v", "--verbose")
        .flag()
        .store_into(opts_buffer_.verbose)
        .help("Output with more verbosity");

    // FIXME: argparse is kind of annoying
    //  maybe want to switch to another lib, or just do it myself

    arg_parser_.add_argument("--stop")
        .choices("never", "first", "each")
        .default_value(std::string{"never"})
        .metavar("WHEN")
        .nargs(1)
        .action([&] (const std::string& opt) {
            using enum ProgramOptions::StopOpt;

            if (opt == "first-error") {
                opts_buffer_.stop_option = FirstError;
            } else if (opt == "each-test-error") {
                opts_buffer_.stop_option = EachTestError;
            } else if (opt == "never") {
                opts_buffer_.stop_option = Never;
            }
        })
        .help("Whether/when to stop early, to not flood the console with failing test messages.");

    arg_parser_.add_argument("-f", "--file")
        .action([this] (const std::string& opt) {
                if (!std::filesystem::exists(opt)) {
                    throw std::invalid_argument(fmt::format("Specified file {:?} does not exist!", opt));
                }
                opts_buffer_.file_name = opt;
        })
        .metavar("FILE")
        .help("File to run tests on");

    arg_parser_.add_argument("-c", "--color")
        .choices("never", "auto", "always")
        .default_value(std::string{"auto"})
        .metavar("WHEN")
        .nargs(1)
        .help("When to use colors")
        .action([this] (const std::string& opt) {
                using enum ProgramOptions::ColorizeOpt;

                if (opt == "never") {
                    opts_buffer_.colorize_option = Never;
                } else if (opt == "auto") {
                    opts_buffer_.colorize_option = Auto;
                } else if (opt == "always") {
                    opts_buffer_.colorize_option = Always;
                }
        });
    // clang-format on
}

util::Expected<ProgramOptions, std::string> CommandLineArgs::parse() {
    parse_successful_ = false;

    try {
        arg_parser_.parse_args(args_);
    } catch (const std::exception& err) {
        return err.what();
    }

    parse_successful_ = true;

    return opts_buffer_;
}

std::string CommandLineArgs::help_message() const {
    return arg_parser_.help().str();
}

std::string CommandLineArgs::usage_message() const {
    return arg_parser_.usage();
}

std::string CommandLineArgs::get_basename(std::string_view full_name) {
    return std::string{full_name.substr(full_name.find_last_of('/') + 1)};
}

ProgramOptions parse_args_or_exit(std::span<const char*> args, int exit_code) {
    CommandLineArgs cl_args{args};
    auto opts_res = cl_args.parse();

    if (!opts_res) {
        fmt::println("{}\n{}", styled(opts_res.error(), fg(fmt::color::red)), cl_args.help_message());
        std::exit(exit_code);
    }

    return opts_res.value();
}
