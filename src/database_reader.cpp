#include "database_reader.hpp"

#include "common/expected.hpp"
#include "grading_session.hpp"
#include "logging.hpp"

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/split.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace asmgrader {

DatabaseReader::DatabaseReader(std::filesystem::path path)
    : path_{std::move(path)} {}

Expected<std::vector<StudentInfo>, std::string> DatabaseReader::read() const {
    std::ifstream in_file{path_};

    if (not in_file.is_open()) {
        return "Failed to open database";
    }

    std::vector<StudentInfo> result;

    std::string line;
    while (std::getline(in_file, line)) {
        // Remove a CR character; windows linefeed
        // Technically, as per RFC4180, we should require that the line ends in CRLF ("\r\n")
        // but that's annoying to the user and there's no good reason to do that.
        // https://datatracker.ietf.org/doc/html/rfc4180
        if (line.ends_with('\r')) {
            line.resize(line.size() - 1);
        }

        // Skip empty lines with a warning
        if (line.empty()) {
            LOG_INFO("Skipping empty line");
            continue;
        }

        std::vector values = line | ranges::views::split(',') | ranges::to<std::vector<std::string>>;

        if (values.size() < 2) {
            return "Too few values in name entry";
        }

        if (values.size() > 2) {
            return "Too many values in name entry";
        }

        StudentInfo entry = {.first_name = values[1],
                             .last_name = values[0],
                             .names_known = true,
                             .assignment_path = std::nullopt,
                             .subst_regex_string = ""};

        result.push_back(std::move(entry));
    }

    if (in_file.bad()) {
        return "IO error in reading";
    }

    return result;
}

} // namespace asmgrader
