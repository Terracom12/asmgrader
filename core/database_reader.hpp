#pragma once

#include "common/expected.hpp"
#include "grading_session.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace asmgrader {

/// Small CSV reader implementation for student names database
/// Expects the specified file to contain a list of newline-seperated
/// "lastname,firstname" entries, utf-8 encoded
class DatabaseReader
{
public:
    explicit DatabaseReader(std::filesystem::path path);

    Expected<std::vector<StudentInfo>, std::string> read() const;

private:
    std::filesystem::path path_;
};

} // namespace asmgrader
