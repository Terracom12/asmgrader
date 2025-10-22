/// \file
/// Provides an API for creating and managing temporary files in the context of tests
#pragma once

#include <asmgrader/common/aliases.hpp>
#include <asmgrader/common/class_traits.hpp>
#include <asmgrader/common/expected.hpp>
#include <asmgrader/logging.hpp>

#include <gsl/pointers>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace asmgrader {

/// Interface onto a temporary file.
class TempFile : NonMovable
{
public:
    explicit TempFile(bool create = true);
    explicit TempFile(u16 perms);
    ~TempFile() noexcept;

    std::string read_all();
    void write(std::string_view str);
    void truncate();

    bool exists() const;

    std::filesystem::path path() const { return file_info_.path; }

    std::string path_str() const { return file_info_.path.string(); }

    [[nodiscard]] static std::filesystem::path unique_path();

    /// Remove (delete) a file. Intended for use with files created based on
    /// \ref unique_path, but could be used to safely remove any arbitrary file.
    static Expected<> remove(const std::filesystem::path& path);

private:
    struct FileInfo
    {
        std::filesystem::path path;
        std::fstream handle;
    };

    [[nodiscard]] static FileInfo generate_unique_file();

    bool ensure_created();

    FileInfo file_info_;
};

} // namespace asmgrader
