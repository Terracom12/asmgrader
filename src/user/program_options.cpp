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

} // namespace asmgrader
