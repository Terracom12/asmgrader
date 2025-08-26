#include "cl_args.hpp"

#include "api/assignment.hpp"
#include "common/expected.hpp"
#include "common/terminal_checks.hpp"
#include "logging.hpp"
#include "program/program.hpp"
#include "registrars/global_registrar.hpp"
#include "user/assignment_file_searcher.hpp"
#include "user/program_options.hpp"
#include "version.hpp"

#include <argparse/argparse.hpp>
#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <gsl/util>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <regex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace asmgrader {

CommandLineArgs::CommandLineArgs(std::span<const char*> args)
    : arg_parser_{get_basename(args[0]), /*unused*/ ASMGRADER_VERSION_STRING, argparse::default_arguments::help}
    , args_{args.begin(), args.end()} {
    // Add parser arguments
    setup_parser();
}

namespace {} // namespace

void CommandLineArgs::setup_parser() {
    if (auto term_sz = terminal_size(stdout)) {
        arg_parser_.set_usage_max_line_width(gsl::narrow_cast<std::size_t>(term_sz->ws_col) * 3 / 4);
        LOG_DEBUG("Cols = {}, px = {}", term_sz->ws_col, term_sz->ws_xpixel);
    } else {
        constexpr std::size_t DEFAULT_MAX_WIDTH = 80;
        LOG_DEBUG("Failed to get terminal size. Setting max width to 80");
        arg_parser_.set_usage_max_line_width(DEFAULT_MAX_WIDTH);
    }

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


        using VerbosityLevelUnderlyingT = std::underlying_type_t<ProgramOptions::VerbosityLevel>;

    arg_parser_.add_argument("-v", "--verbose")
        .flag()
        .action([this] (const std::string& /*unused*/) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                static auto& verbosity_ref = reinterpret_cast<VerbosityLevelUnderlyingT&>(opts_buffer_.verbosity);

                verbosity_ref++;
            })
        .append()
        .help("Increase verbosity level");

    arg_parser_.add_argument("-q", "--quiet")
        .flag()
        .action([this] (const std::string& /*unused*/) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                static auto& verbosity_ref = reinterpret_cast<VerbosityLevelUnderlyingT&>(opts_buffer_.verbosity);

                verbosity_ref--;
            })
        .append()
        .help("Decrease verbosity level");

    arg_parser_.add_argument("--silent")
        .flag()
        .action([this] (const std::string& /*unused*/) {
                opts_buffer_.verbosity = ProgramOptions::VerbosityLevel::Silent;
            })
        .help("Sets verbosity level to 'Silent', suppressing all output except for the return code. Useful for scripting.");

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
    // Manual implementation of mutually exclusive group, as argparse doesn't seem to support
    // such for groups of arguments.

    arg_parser_.add_argument("-fm", "--file-matcher")
        .default_value(std::string{ProgramOptions::DEFAULT_FILE_MATCHER})
        .nargs(1)
        .metavar("REGEX")
        .action([this] (const std::string& opt) {
                opts_buffer_.file_matcher = opt;
        })
        .help("RegEx to match files for a given student and assignment.\nSee docs for syntax details.");

    arg_parser_.add_argument("-db", "--database")
        .default_value(std::string{ProgramOptions::DEFAULT_DATABASE_PATH})
        .nargs(1)
        .metavar("FILE")
        .action([this] (const std::string& opt) {
                opts_buffer_.database_path = opt;
        })
        .help("CSV database file with student names. If not specified, "
              "will attempt to find student submissions recursively using heuristics.\nSee docs for format spec.");

    arg_parser_.add_argument("-p", "--search-path")
        .default_value(std::string{ProgramOptions::DEFAULT_SEARCH_PATH})
        .nargs(1)
        .metavar("PATH")
        .action([this] (const std::string& opt) {
                opts_buffer_.search_path = opt;
        })
        .help("Root path to begin searching for student assignments.");
#endif // PROFESSOR_VERSION

    arg_parser_.add_argument("-f", "--file")
        .metavar("FILE")
        .action([this] (const std::string& opt) {
                opts_buffer_.file_name = opt;
        })
        .help(APP_MODE == AppMode::Professor ?
                "The *individual* file to run tests on. No other files are searched for, nor is the database read.\n"
                "This argument's behavior overrides any usage of --file-matcher, --search-path, and --database." :  // professor help msg
                "The file to run tests on." // student help msg
        );
    // clang-format on
}

Expected<ProgramOptions, std::string> CommandLineArgs::parse() {
    parse_successful_ = false;

    try {
        arg_parser_.parse_args(args_);
    } catch (const std::exception& err) {
        return err.what();
    }

    if (auto valid_res = opts_buffer_.validate(); valid_res.has_error()) {
        return valid_res.error();
    }

    parse_successful_ = true;

    LOG_DEBUG("Parsed CLI arguments: {}", opts_buffer_);

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

ProgramOptions parse_args_or_exit(std::span<const char*> args, int exit_code) noexcept {
    CommandLineArgs cl_args{args};
    auto opts_res = cl_args.parse();

    if (!opts_res) {
        fmt::println("{}\n{}", styled(opts_res.error(), fg(fmt::color::red)), cl_args.help_message());
        std::exit(exit_code);
    }

    return opts_res.value();
}

} // namespace asmgrader
