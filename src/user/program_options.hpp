#pragma once

#include <asmgrader/common/formatters/debug.hpp>
#include <asmgrader/registrars/global_registrar.hpp>

#include "common/error_types.hpp"
#include "common/expected.hpp"
#include "output/verbosity.hpp"
#include "program/program.hpp"
#include "user/assignment_file_searcher.hpp"
#include "version.hpp"

#include <fmt/base.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <libassert/assert.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>

namespace asmgrader {

struct ProgramOptions
{

    // ###### Argument fields

    /// Level of verbosity for cli output.
    /// See \ref verbosity_levels_desc for an explaination of each of the levels.
    VerbosityLevel verbosity = DEFAULT_VERBOSITY_LEVEL;
    std::string assignment_name;

    /// Filter for test cases to be ran
    /// Only very basic matching for now: simply checks whether the specified string exists anywhere
    /// within the test case name.
    std::optional<std::string> tests_filter;

    /// Never = stop only on fatal errors
    /// FirstError = stop completely on the first error encountered
    /// EachTestError = stop each test early upon error, but still attempt to run subsequent tests
    enum class StopOpt { Never, FirstError, EachTestError } stop_option;

    enum class ColorizeOpt { Auto, Always, Never } colorize_option;

    // TODO: Premit simplified execution of individual files in prof mode. Has to be mutually excusive with some
    // other opts

    // Student version only
    std::optional<std::string> file_name;

    // PROFESSOR_VERSION only
    std::string file_matcher = std::string{DEFAULT_FILE_MATCHER};
    std::filesystem::path database_path = DEFAULT_DATABASE_PATH;
    std::filesystem::path search_path = DEFAULT_SEARCH_PATH;

    // ###### Argument defaults

    static constexpr std::string_view DEFAULT_DATABASE_PATH = "students.csv";
    static constexpr std::string_view DEFAULT_SEARCH_PATH = ".";
    static constexpr std::string_view DEFAULT_FILE_MATCHER = AssignmentFileSearcher::DEFAULT_REGEX;
    static constexpr auto DEFAULT_VERBOSITY_LEVEL = VerbosityLevel::Summary;

    static Expected<void, std::string> ensure_file_exists(const std::filesystem::path& path,
                                                          fmt::format_string<std::string> fmt) {
        if (!std::filesystem::exists(path)) {
            return (fmt::format(fmt, path.string()) + " does not exist");
        }

        return {};
    }

    static Expected<void, std::string> ensure_is_regular_file(const std::filesystem::path& path,
                                                              fmt::format_string<std::string> fmt) {
        TRY(ensure_file_exists(path, fmt));

        if (!std::filesystem::is_regular_file(path)) {
            return (fmt::format(fmt, path.string()) + " is not a regular file");
        }

        return {};
    }

    [[maybe_unused]] static Expected<void, std::string> ensure_is_directory(const std::filesystem::path& path,
                                                                            fmt::format_string<std::string> fmt) {
        TRY(ensure_file_exists(path, fmt));

        if (!std::filesystem::is_directory(path)) {
            return (fmt::format(fmt, path.string()) + " is not a directory");
        }

        return {};
    }

    /// Verify that all fields are valid
    Expected<void, std::string> validate() {
        // Assume that all enumerators have valid values except for verbosity
        // which we will just clamp to [MIN, MAX]

        constexpr auto MAX_VERBOSITY = VerbosityLevel::Max;
        constexpr auto MIN_VERBOSITY = VerbosityLevel{};

        verbosity = std::clamp(verbosity, MIN_VERBOSITY, MAX_VERBOSITY);

        // Ensure that the matcher is a valid RegEx
        try {
            std::ignore = std::regex{file_matcher};
        } catch (std::exception& ex) {
            return (fmt::format("File matcher {:?} is invalid. {}", file_matcher, ex.what()));
        }

        TRY(ensure_is_directory(search_path, "Search path {:?}"));

        // Only check the database path if it's not the default
        // non-existance will be handled properly in ProfessorApp
        if (database_path != DEFAULT_DATABASE_PATH) {
            TRY(ensure_is_regular_file(database_path, "Database file {:?}"));
        }

        // If the assignment name is empty, we're going to be attempting to infer it elsewhere
        if (assignment_name.empty()) {
            return {};
        }

        // The CLI should verify that the specified assignment is valid
        // We'll check here just in case and return an error if it's not
        auto assignment = TRYE(GlobalRegistrar::get().get_assignment(assignment_name),
                               fmt::format("Error locating assignment {}", assignment_name));

        if (APP_MODE != AppMode::Professor) {
            // TODO: A more friendly diagnostic for non-existant file
            std::string exec_file_name = file_name.value_or(assignment.get().get_exec_path());

            TRY(ensure_is_regular_file(exec_file_name, "File to run tests on {:?}"));
            TRY(Program::check_is_compat_elf(exec_file_name));
        }

        return {};
    }
};

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::ProgramOptions> : ::asmgrader::DebugFormatter
{
    auto format(const ::asmgrader::ProgramOptions& from, fmt::format_context& ctx) const {
        // TODO: Enums -> strings

        ctx.advance_to(
            fmt::format_to(ctx.out(), "{{verbosity={}, assignment={}, stop_opt={}, color_opt={}, file_name={}",
                           fmt::underlying(from.verbosity), from.assignment_name, fmt::underlying(from.stop_option),
                           fmt::underlying(from.colorize_option), from.file_name));

        if (asmgrader::APP_MODE == asmgrader::AppMode::Professor) {
            return fmt::format_to(ctx.out(), " file_matcher={}, database_path={}, search_path={}", from.file_matcher,
                                  from.database_path, from.search_path);
        }

        return ctx.out() = '}';
    }
};
