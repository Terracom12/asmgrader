/// \file
/// Provides an API for basic process statistics
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <sched.h>

namespace asmgrader {

/// Obtains statistics of a process with a given PID.
/// Provides various accessors returning POD types.
///
/// For now, features are extremely limited.
///
/// Makes use of proc(5)
class ProcessStats
{
public:
    /// Open File Descriptor(s) Info POD
    struct OpenFds
    {
        std::vector<int> fds;
    };

    explicit ProcessStats(pid_t pid);

    OpenFds open_fds() const { return open_fds_; }

private:
    std::filesystem::path procfs_path(std::string_view suffix) const;

    OpenFds get_open_fds() const;

    pid_t pid_;

    OpenFds open_fds_;
};

} // namespace asmgrader
