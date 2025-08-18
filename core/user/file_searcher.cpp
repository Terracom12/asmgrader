#include "user/file_searcher.hpp"

#include "logging.hpp"

#include <range/v3/algorithm/replace.hpp>

#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

FileSearcher::FileSearcher(std::string expr, std::map<std::string, std::string> args)
    : expr_{std::move(expr)}
    , args_{std::move(args)} {}

std::string FileSearcher::set_arg(const std::string& key, std::string_view value) {
    if (auto iter = args_.find(key); iter != args_.end()) {
        return std::exchange(iter->second, value);
    }

    args_[key] = value;
    return "";
}

std::vector<std::filesystem::path> FileSearcher::search(const std::filesystem::path& base) {
    return search_recursive(base, 1);
}

std::vector<std::filesystem::path> FileSearcher::search_recursive(const std::filesystem::path& base, int max_depth) {
    namespace fs = std::filesystem;

    std::vector<std::filesystem::path> result;

    std::regex regex{subst_args()};

    for (auto iter = fs::recursive_directory_iterator{base}; iter != fs::recursive_directory_iterator{}; ++iter) {
        if (!iter->is_regular_file()) {
            continue;
        }

        if (iter.depth() > max_depth) {
            iter.pop();
        }

        fs::path path = iter->path();

        if (does_match(regex, path.filename().c_str())) {
            result.push_back(path);
        }
    }

    return result;
}

std::string FileSearcher::subst_args() const {
    std::string result = expr_;

    for (const auto& [key, value] : args_) {
        std::regex regex_var_expr{"`" + key + "`"};
        result = std::regex_replace(result, regex_var_expr, value);
    }

    return result;
}

bool FileSearcher::does_match(const std::regex& expr, std::string_view filename) {
    return std::regex_match(filename.begin(), filename.end(), expr);
}
