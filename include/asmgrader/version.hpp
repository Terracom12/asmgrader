#pragma once

#include <asmgrader/app_mode.hpp>
#include <asmgrader/common/formatters/debug.hpp>
#include <asmgrader/common/formatters/macros.hpp>
#include <asmgrader/common/os.hpp>
#include <asmgrader/common/static_string.hpp>
#include <asmgrader/version_macros.hpp> // IWYU pragma: export

#include <boost/preprocessor/stringize.hpp>
#include <fmt/base.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gsl/narrow>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/iterator.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/split.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace asmgrader::buildinfo {

struct Dependency
{
    std::string_view name;
    std::string_view version;
};

struct CompilerInfo
{
    /// Compiler vendor (GCC or Clang)
    enum Vendor { Unknown, GCC, Clang } vendor;

    /// Major version number
    int major;
    /// Minor version number
    int minor;
    /// Patch version number
    int patch;
};

struct BuildInfo
{
    /// Major version number
    int major;

    /// Minor version number
    int minor;

    /// Patch version number
    int patch;

    /// Hex characters of the latest git commit's hash
    std::string_view git_hash;

    std::string_view build_type;

    ProcessorKind system_processor;
    EndiannessKind endianness;

    CompilerInfo compiler_info;

    AppMode app_mode;

    /// Name of the implementation using the library
    std::string_view impl_name;
    /// Version of the implementation using the library
    std::string_view impl_version;

    /// C++ standard we compiled with (__cplusplus)
    long cpp_standard; // NOLINT(google-runtime-int)

    /// Date of compilation (__DATE__)
    std::string_view date;
    /// Time of compilation (__TIME__)
    std::string_view time;

    /// Library dependencies of the project
    /// Use \ref get_dependencies for a more structured form of dependency info
    /// Consists of a number of space seperated "<libname> <version>" strings
    /// Example: "fmt 11.1.3 spdlog 1.15.1" ...
    std::string_view dependencies;
};

std::string_view format_as(const AppMode& from);

std::string_view format_as(const CompilerInfo::Vendor& from);

buildinfo::BuildInfo get_build_info();

/// \returns std::vector<buildinfo::Dependency> for all of the dependencies found
/// in \ref get_build_info().dependencies
std::vector<Dependency> get_dependencies();

const char* get_plain_version_str();

std::string get_version_str();

} // namespace asmgrader::buildinfo

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output
template <>
struct fmt::formatter<asmgrader::buildinfo::Dependency> : asmgrader::DebugFormatter
{
    fmt::appender format(const asmgrader::buildinfo::Dependency& from, format_context& ctx) const;
};

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output
template <>
struct fmt::formatter<asmgrader::buildinfo::CompilerInfo> : asmgrader::DebugFormatter
{
    fmt::appender format(const asmgrader::buildinfo::CompilerInfo& from, format_context& ctx) const;
};

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output
template <>
struct fmt::formatter<asmgrader::buildinfo::BuildInfo> : asmgrader::DebugFormatter
{
    fmt::appender format(const asmgrader::buildinfo::BuildInfo& from, format_context& ctx) const;
};
