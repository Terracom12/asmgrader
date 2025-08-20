#pragma once

#include "grading_session.hpp"
#include "common/expected.hpp"

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

    util::Expected<std::vector<StudentInfo>, std::string> read() const;

private:
    std::filesystem::path path_;
};
