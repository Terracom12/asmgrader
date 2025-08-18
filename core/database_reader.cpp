#include "database_reader.hpp"

#include "logging.hpp"
#include "util/expected.hpp"

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/split.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

DatabaseReader::DatabaseReader(std::filesystem::path path)
    : path_{std::move(path)} {}

util::Expected<DatabaseReader::ReadResult, std::string> DatabaseReader::read() const {
    std::ifstream in_file{path_};

    if (not in_file.is_open()) {
        return "Failed to open database file";
    }

    ReadResult result;

    std::string line;
    while (std::getline(in_file, line)) {
        // Skip empty lines with a warning
        if (line.empty()) {
            LOG_WARN("Skipping empty line in database file");
            continue;
        }

        std::vector values = line | ranges::views::split(',') | ranges::to<std::vector<std::string>>;

        if (values.size() < 2) {
            return "Too few values in name entry in database";
        }

        if (values.size() > 2) {
            return "Too many values in name entry in database";
        }

        NameEntry entry = {.first_name = values[1], .last_name = values[0]};

        result.entries.push_back(std::move(entry));
    }

    if (in_file.bad()) {
        return "IO error in reading database file";
    }

    return result;
}
