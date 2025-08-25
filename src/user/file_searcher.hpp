#pragma once

#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace asmgrader {

class FileSearcher
{
public:
    explicit FileSearcher(std::string expr, std::map<std::string, std::string> args = {});

    /// Equivalent to `search_recursive(base, 1)`
    std::vector<std::filesystem::path> search(const std::filesystem::path& base);

    std::vector<std::filesystem::path> search_recursive(const std::filesystem::path& base, int max_depth);

protected:
    std::string set_arg(const std::string& key, std::string_view value);

private:
    std::string subst_args() const;
    static bool does_match(const std::regex& expr, std::string_view filename);

    std::string expr_;
    std::map<std::string, std::string> args_;
};

} // namespace asmgrader
