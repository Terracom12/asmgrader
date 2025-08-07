#pragma once

#include "user/program_options.hpp"
#include "util/expected.hpp"

#include <argparse/argparse.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/// Just a wrapper around argparse for now
class CommandLineArgs
{
public:
    explicit CommandLineArgs(std::span<const char*> args);

    /// Returns:
    ///   Success - Expected<ProgramOptions> with parsed program options structure
    ///   Failure - Expected<std::string> with failure essage
    util::Expected<ProgramOptions, std::string> parse();

    std::string usage_message() const;
    std::string help_message() const;

    std::optional<ProgramOptions> get_options() const;

private:
    /// Set up the ArgumentParser for fields of ProgramOptions
    void setup_parser();

    /// Obtain the basename of a full pathname
    /// Used for the program name with argparse
    static std::string get_basename(std::string_view full_name);

    argparse::ArgumentParser arg_parser_;
    std::vector<std::string> args_;

    ProgramOptions opts_buffer_ = {};

    bool parse_successful_ = false;
};

ProgramOptions parse_args_or_exit(std::span<const char*> args, int exit_code = 1);
