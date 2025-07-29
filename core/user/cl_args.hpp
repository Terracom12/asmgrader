#pragma once

#include "util/expected.hpp"

#include <argparse/argparse.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ProgramArgsSingleton;

/// Just a wrapper around argparse for now
class CommandLineArgs
{
public:
    struct ProgramOptions
    {
        bool verbose;
        std::string assignment_name;
        std::optional<std::string> file_name;

        /// Never = stop only on fatal errors
        /// FirstError = stop completely on the first error encountered
        /// EachTestError = stop each test early upon error, but still attempt to run subsequent tests
        enum { Never, FirstError, EachTestError } stop_option;
    };

    explicit CommandLineArgs(std::span<const char*> args);

    /// Returns:
    ///   Success - Expected<ProgramOptions> with parsed program options structure
    ///   Failure - Expected<std::string> with failure essage
    util::Expected<void, std::string> parse();

    std::string usage_message() const;
    std::string help_message() const;

    std::optional<ProgramOptions> get_options() const;

    /// Sets ``ProgramArgsSingleton`` based on the previously parsed options
    /// Resets the global to nullopt if a parse has not yet occurred, or the previous parse was not a success
    void set_global() const;

private:
    /// Set up the ArgumentParser for fields of ProgramOptions
    void setup_parser();

    /// Obtain the basename of a full pathname
    /// Used for the program name with argparse
    static std::string_view get_basename(std::string_view full_name);

    argparse::ArgumentParser arg_parser_;
    std::vector<std::string> args_;

    ProgramOptions opts_buffer_ = {};

    bool parse_successful_ = false;
};

class ProgramArgsSingleton
{
public:
    static const std::optional<CommandLineArgs::ProgramOptions>& get_options() noexcept;

    friend class CommandLineArgs;

private:
    static void set_options(std::optional<CommandLineArgs::ProgramOptions> options) noexcept;

    static inline std::optional<CommandLineArgs::ProgramOptions> opts{};
};

inline const std::optional<CommandLineArgs::ProgramOptions>& program_args = ProgramArgsSingleton::get_options();
