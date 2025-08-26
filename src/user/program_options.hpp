#pragma once

#include <asmgrader/common/formatters/debug.hpp>

#include "user/assignment_file_searcher.hpp"
#include "version.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace asmgrader {

struct ProgramOptions
{
    static constexpr std::string_view DEFAULT_DATABASE_PATH = "students.csv";
    static constexpr std::string_view DEFAULT_SEARCH_PATH = ".";
    static constexpr std::string_view DEFAULT_FILE_MATCHER = AssignmentFileSearcher::DEFAULT_REGEX;

    /// Levels are explained in `verbosity.hpp`
    enum class VerbosityLevel { Silent, Quiet, Summary, FailsOnly, All, Extra, Max } verbosity;
    std::string assignment_name;

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
    std::optional<std::filesystem::path> database_path = DEFAULT_DATABASE_PATH;
    std::filesystem::path search_path = DEFAULT_SEARCH_PATH;
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
