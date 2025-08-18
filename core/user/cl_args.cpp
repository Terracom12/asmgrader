#include "cl_args.hpp"

#include "api/assignment.hpp"
#include "registrars/global_registrar.hpp"
#include "user/assignment_file_searcher.hpp"
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
#include <regex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

CommandLineArgs::CommandLineArgs(std::span<const char*> args)
    : arg_parser_{get_basename(args[0]), /*unused*/ ASMGRADER_VERSION_STRING, argparse::default_arguments::help}
    , args_{args.begin(), args.end()} {
    // Add parser arguments
    setup_parser();
}

namespace {

void ensure_regular_file(const std::filesystem::path& path, fmt::format_string<std::string> fmt) {
    if (!std::filesystem::exists(path)) {
        throw std::invalid_argument(fmt::format(fmt, path.string()) + " does not exist");
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw std::invalid_argument(fmt::format(fmt, path.string()) + " is not a regular file");
    }
}

} // namespace

void CommandLineArgs::setup_parser() {
    const auto assignment_names =
        GlobalRegistrar::get().for_each_assignment([&](const Assignment& assignment) { return assignment.get_name(); });

    constexpr auto VERSION_FMT = "AsmGrader v{}"
#ifdef PROFESSOR_VERSION
                                 " (Professor's Version)"
#endif
        ;
    arg_parser_.add_description(fmt::format(VERSION_FMT, ASMGRADER_VERSION_STRING));

    // FIXME: argparse is kind of annoying. Behavior is dependant upon ORDER of chained fn calls.
    //  maybe want to switch to another lib, or just do it myself. Need arg choices in help.

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
        .default_value(false)
        .implicit_value(true)
        .nargs(0)
        .action([&](const auto & /*unused*/) {
            fmt::println(ASMGRADER_VERSION_STRING);
            std::exit(0);
        })
        .help("prints version information and exits");

    arg_parser_.add_argument("-v", "--verbose")
        .flag()
        .store_into(opts_buffer_.verbose)
        .help("Output with more verbosity");

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

#ifdef PROFESSOR_VERSION
    arg_parser_.add_argument("-fm", "--file-matcher")
        .default_value(std::string{AssignmentFileSearcher::DEFAULT_REGEX})
        .nargs(1)
        .metavar("REGEX")
        .action([this] (const std::string& opt) {
                // Ensure that the matcher is a valid RegEx
                try {
                    std::ignore = std::regex{opt};
                } catch (std::exception& ex) {
                    throw std::invalid_argument(fmt::format("File matcher {:?} is invalid. {}", opt, ex.what()));
                }
                opts_buffer_.file_matcher = opt;
        })
        .help("RegEx to match files for a given student and assignment.\nSee docs for syntax details.");

    arg_parser_.add_argument("-db", "--database")
        .nargs(1)
        .metavar("FILE")
        .action([this] (const std::string& opt) {
                ensure_regular_file(opt, "Database file {:?}");

                opts_buffer_.database_path = opt;
        })
        .help("CSV database file with student names. If not specified, "
              "will attempt to find student submissions recursively using heuristics.\nSee docs for format spec.");
#else // PROFESSOR_VERSION
    arg_parser_.add_argument("-f", "--file")
        .metavar("FILE")
        .action([this] (const std::string& opt) {
                ensure_regular_file(opt, "File to run tests on {:?}");
                opts_buffer_.file_name = opt;
        })
        .help("File to run tests on");
#endif // PROFESSOR_VERSION
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
