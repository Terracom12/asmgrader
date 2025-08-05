#include "cl_args.hpp"

#include "registrars/global_registrar.hpp"

#include <fmt/color.h>
#include <fmt/format.h>

#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

CommandLineArgs::CommandLineArgs(std::span<const char*> args)
    : arg_parser_{args[0], "0.0.1", argparse::default_arguments::help}
    , args_{args.begin(), args.end()} {
    // Add parser arguments
    setup_parser();
}

void CommandLineArgs::setup_parser() {
    const auto assignment_names =
        GlobalRegistrar::get().for_each_assignment([&](const Assignment& assignment) { return assignment.get_name(); });

    // clang-format off
    auto& assignment_arg = arg_parser_.add_argument("assignment")
        .store_into(opts_buffer_.assignment_name)
        // Add all assignment names to help msg, since argparse won't add choices by
        // default for some reason
        .help(fmt::format("The assignment to run tests on\nOne of: {:n}", assignment_names));
    GlobalRegistrar::get().for_each_assignment([&](const Assignment& assignment) {
        assignment_arg.add_choice(assignment.get_name());
    });

    arg_parser_.add_argument("-v", "--verbose")
        .flag()
        .store_into(opts_buffer_.verbose)
        .help("Output with more verbosity");

    // FIXME: argparse is kind of annoying
    //  maybe want to switch to another lib, or just do it myself

    arg_parser_.add_argument("--stop")
        .default_value(std::string{"never"})
        .choices("never", "first", "each")
        .metavar("WHEN")
        .nargs(1)
        .action([&] (const std::string& opt) {
            if (opt == "first-error") {
                opts_buffer_.stop_option = ProgramOptions::FirstError;
            } else if (opt == "each-test-error") {
                opts_buffer_.stop_option = ProgramOptions::EachTestError;
            } else if (opt == "never") {
                opts_buffer_.stop_option = ProgramOptions::Never;
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
    // clang-format on
}

util::Expected<void, std::string> CommandLineArgs::parse() {
    parse_successful_ = false;

    try {
        arg_parser_.parse_args(args_);
        // LOG_TRACE("Program options: {}", opts_buffer_);
    } catch (const std::exception& err) {
        return err.what();
    }

    parse_successful_ = true;

    return {};
}

std::string CommandLineArgs::help_message() const {
    return arg_parser_.help().str();
}
std::string CommandLineArgs::usage_message() const {
    return arg_parser_.usage();
}

std::string_view CommandLineArgs::get_basename(std::string_view full_name) {
    return full_name.substr(full_name.find_last_of('/') + 1);
}

void CommandLineArgs::set_global() const {
    if (!parse_successful_) {
        return;
    }

    ProgramArgsSingleton::set_options(opts_buffer_);
}

const std::optional<CommandLineArgs::ProgramOptions>& ProgramArgsSingleton::get_options() noexcept {
    return opts;
}

void ProgramArgsSingleton::set_options(std::optional<CommandLineArgs::ProgramOptions> options) noexcept {
    opts = std::move(options);
}
