#pragma once

#include <filesystem>
#include <optional>
#include <string>

struct ProgramOptions
{
    /// Names are mostly self-explanatory
    ///   not entirely sure what "Max" will do yet, but it's there if needed
    enum class VerbosityLevel { Silent, Quiet, Summary, FailsOnly, All, ExtraInfo, Max } verbosity;
    std::string assignment_name;

    /// Never = stop only on fatal errors
    /// FirstError = stop completely on the first error encountered
    /// EachTestError = stop each test early upon error, but still attempt to run subsequent tests
    enum class StopOpt { Never, FirstError, EachTestError } stop_option;

    enum class ColorizeOpt { Auto, Always, Never } colorize_option;

    // Student version only
    std::optional<std::string> file_name;

    // PROFESSOR_VERSION only
    std::string file_matcher;
    std::optional<std::filesystem::path> database_path;
    std::filesystem::path search_path = ".";
};
