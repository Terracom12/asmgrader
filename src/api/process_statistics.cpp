#include <asmgrader/api/process_statistics.hpp>

#include <asmgrader/logging.hpp>

#include <fmt/format.h>

#include <filesystem>
#include <string>
#include <string_view>

#include <sched.h>

namespace asmgrader {

namespace fs = std::filesystem;

ProcessStats::ProcessStats(pid_t pid)
    : pid_{pid}
    , open_fds_{get_open_fds()} {}

fs::path ProcessStats::procfs_path(std::string_view suffix) const {
    constexpr std::string_view procfs_template = "/proc/{}";
    fs::path base_path = fmt::format(procfs_template, pid_);
    return base_path / suffix;
}

ProcessStats::OpenFds ProcessStats::get_open_fds() const {
    const fs::path procfs_fds = procfs_path("fd/");

    if (!fs::exists(procfs_fds)) {
        LOG_WARN("Could not get open fds : path {} does not exist (pid={})", procfs_fds, pid_);
        return {};
    }

    OpenFds res;

    for (const auto& dir_entry : fs::directory_iterator{procfs_fds}) {
        std::string filename = dir_entry.path().filename().string();

        LOG_DEBUG("Got filename: {:?}", filename);

        try {
            res.fds.push_back(std::stoi(filename));
        } catch (...) {
            res.fds.push_back(-1);
        }
    }

    return res;
}

} // namespace asmgrader
