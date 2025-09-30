#include "user/program_options.hpp"

#include "common/error_types.hpp"
#include "common/expected.hpp"
#include "output/verbosity.hpp"
#include "program/program.hpp"
#include "registrars/global_registrar.hpp"
#include "version.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <regex>
#include <string>
#include <tuple>

namespace asmgrader {

Expected<void, std::string> ProgramOptions::ensure_file_exists(const std::filesystem::path& path,
                                                               fmt::format_string<std::string> fmt) {
    if (!std::filesystem::exists(path)) {
        return (fmt::format(fmt, path.string()) + " does not exist");
    }

    return {};
}

Expected<void, std::string> ProgramOptions::ensure_is_regular_file(const std::filesystem::path& path,
                                                                   fmt::format_string<std::string> fmt) {
    TRY(ensure_file_exists(path, fmt));

    if (!std::filesystem::is_regular_file(path)) {
        return (fmt::format(fmt, path.string()) + " is not a regular file");
    }

    return {};
}

Expected<void, std::string> ProgramOptions::ensure_is_directory(const std::filesystem::path& path,
                                                                fmt::format_string<std::string> fmt) {
    TRY(ensure_file_exists(path, fmt));

    if (!std::filesystem::is_directory(path)) {
        return (fmt::format(fmt, path.string()) + " is not a directory");
    }

    return {};
}

Expected<void, std::string> ProgramOptions::validate() {
    // Assume that all enumerators have valid values except for verbosity
    // which we will just clamp to [MIN, MAX]

    constexpr auto max_verbosity = VerbosityLevel::Max;
    constexpr auto min_verbosity = VerbosityLevel{};

    verbosity = std::clamp(verbosity, min_verbosity, max_verbosity);

    // Ensure that the matcher is a valid RegEx
    try {
        std::ignore = std::regex{file_matcher};
    } catch (std::exception& ex) {
        return (fmt::format("File matcher {:?} is invalid. {}", file_matcher, ex.what()));
    }

    TRY(ensure_is_directory(search_path, "Search path {:?}"));

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

    // Only check the database path if it's not the default
    // non-existance will be handled properly in ProfessorApp
    if (database_path != DEFAULT_DATABASE_PATH) {
        TRY(ensure_is_regular_file(database_path, "Database file {:?}"));
    }

    return {};
}

} // namespace asmgrader
