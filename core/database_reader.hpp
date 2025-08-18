#pragma once

#include "util/expected.hpp"

#include <filesystem>
#include <string>
#include <vector>

/// Small CSV reader implementation for student names database
/// Expects the specified file to contain a list of newline-seperated
/// "lastname,firstname" entries, utf-8 encoded
class DatabaseReader
{
public:
    explicit DatabaseReader(std::filesystem::path path);

    struct NameEntry
    {
        std::string first_name;
        std::string last_name;
    };

    struct ReadResult
    {
        std::vector<NameEntry> entries;
    };

    util::Expected<ReadResult, std::string> read() const;

private:
    std::filesystem::path path_;
};
