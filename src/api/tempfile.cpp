#include "api/tempfile.hpp"

#include "common/aliases.hpp"
#include "common/expected.hpp"
#include "logging.hpp"

#include <libassert/assert.hpp>
#include <range/v3/algorithm/generate.hpp>
#include <range/v3/algorithm/shuffle.hpp>
#include <range/v3/algorithm/transform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace asmgrader {

namespace fs = std::filesystem;

TempFile::TempFile()
    : file_info_{generate_unique_file()} {}

TempFile::TempFile(u16 perms)
    : TempFile() {
    auto fs_perms = static_cast<fs::perms>(perms);
    if ((fs_perms & ~fs::perms::mask) != fs::perms::none) {
        LOG_ERROR("Invalid file perms specified ({:o}); Not setting permissions.", perms);
        return;
    }

    std::error_code err;
    fs::permissions(file_info_.path, fs_perms, err);
    if (err != std::error_code{}) {
        LOG_ERROR("Failed to set file permissions to {:o} for {} : {}", perms, file_info_.path, err);
    }
}

TempFile::~TempFile() noexcept {
    std::error_code err;
    fs::remove(file_info_.path, err);
    if (err != std::error_code{}) {
        LOG_ERROR("Failed to remove tempfile {} : {}", file_info_.path, err);
    }
}

std::string TempFile::read_all() {
    // seek to the beginning of the file
    file_info_.handle.seekg(0);

    std::stringstream strm;
    strm << file_info_.handle.rdbuf();
    return strm.str();
}

void TempFile::write(std::string_view str) {
    // seek to the end of the file
    file_info_.handle.seekg(0, std::ios::end);
    file_info_.handle << str;
}

void TempFile::truncate() {
    std::error_code err;
    fs::resize_file(file_info_.path, 0, err);
    if (err != std::error_code{}) {
        LOG_ERROR("Failed truncate tempfile {} : {}", file_info_.path, err);
    }
}

std::filesystem::path TempFile::unique_path() {
    // Adapted from https://stackoverflow.com/a/79631059
    // Courtesy of Ted Lyngmo
    // NOLINTBEGIN

    static const auto chars = [] {
        // the characters you'd like to include
        auto arr =
            std::to_array({'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                           's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                           'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'});
        // randomize the order of the characters:
        ranges::shuffle(arr, std::mt19937(std::random_device{}()));
        return arr;
    }();

    constexpr auto length_of_basename = 8;

    thread_local auto baseidx = [&] {
        // randomize the initial state:
        std::array<uint_least8_t, length_of_basename> rv{};
        std::mt19937 prng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
        ranges::generate(rv, [&] { return static_cast<uint_least8_t>(dist(prng)); });
        return rv;
    }();

    // create the basename from the baseidices:
    thread_local auto basename = [&] {
        std::array<char, length_of_basename + 1> rv{};
        ranges::transform(baseidx, rv.data(), [&](auto idx) { return chars[idx]; });
        return rv;
    }();

    fs::path tmpdir = fs::temp_directory_path();

    constexpr auto num_tries = 10'000;

    for (int i = 0; i < num_tries; ++i) { // try a few different filenames
        // generate the next basename:
        size_t idx = length_of_basename;

        do {
            --idx;
            baseidx[idx] = (baseidx[idx] + 1) % chars.size();
            basename[idx] = chars[baseidx[idx]];
        } while (idx && baseidx[idx] == 0);

        // ...and create a full path:
        fs::path tmpfile_path = tmpdir / basename.data();

        // < C++23, so ios::noreplace is not available
        if (!fs::exists(tmpfile_path)) {
            return tmpfile_path;
        }
    }
    // NOLINTEND

    LOG_FATAL("Failed to generate a unique filename tmpdir={} | num_tries={} | baseidx={} | basename={}", tmpdir,
              num_tries, baseidx, basename);
    throw std::runtime_error("Failed to generate a unique filename");
}

TempFile::FileInfo TempFile::generate_unique_file() {
    TempFile::FileInfo ret{};

    // FIXME: technically could have a race condition
    auto path = unique_path();

    ret.handle.open(path, std::ios::in | std::ios::out | std::ios::trunc);
    ret.path = std::move(path);

    LOG_DEBUG("Creating a temporary file at {}", ret.path);
    if (!ret.handle) {
        LOG_FATAL("Failed to create temporary file {}", ret.path);
        throw std::runtime_error("Failed to create temporary file");
    }

    return ret;
}

Expected<> TempFile::remove(const fs::path& path) {
    std::error_code err;
    fs::remove(path, err);

    if (err != std::error_code{}) {
        LOG_ERROR("Failed remove file {} : {}", path, err);
        return err;
    }

    return {};
}

} // namespace asmgrader
