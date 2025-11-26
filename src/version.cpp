// ccache:disable
// Rational: __DATE__ and __TIME__ macros should correspond to actual build date and time
#include "version.hpp"

#include "common/os.hpp"
#include "common/static_string.hpp"

#include <boost/preprocessor/stringize.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/range/conversion.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace asmgrader::buildinfo {

std::string_view format_as(const CompilerInfo::Vendor& from) {
    using enum CompilerInfo::Vendor;
    switch (from) {
    case GCC:
        return "GCC";
    case Clang:
        return "Clang";
    case Unknown:
    default:
        return "<unknown>";
    }
};

/// Implementation details for creating a canonical BuildInfo instance
namespace detail {

consteval CompilerInfo make_compiler_info() {
    CompilerInfo compiler_info{};

#if defined(__GNUC__) && !defined(__clang__)
    compiler_info.vendor = CompilerInfo::GCC;
    compiler_info.major = __GNUC__;
    compiler_info.minor = __GNUC_MINOR__;
    compiler_info.patch = __GNUC_PATCHLEVEL__;
#elif defined(__clang__)
    compiler_info.vendor = CompilerInfo::Clang;
    compiler_info.major = __clang_major__;
    compiler_info.minor = __clang_minor__;
    compiler_info.patch = __clang_patchlevel__;
#endif

    return compiler_info;
}

consteval AppMode make_app_mode() {
    return get_builtin_app_mode();
}

constexpr StaticString static_deps_str{ASMGRADER_DEPENDENCY_VERSIONS_STR};

consteval std::string_view make_impl_name() {
    std::string_view impl_version_str = ASMGRADER_IMPL_VERSION_INFO;

    std::size_t space_pos = impl_version_str.find(' ');

    if (space_pos == std::string_view::npos) {
        return "(name unknown)";
    }

    return impl_version_str.substr(0, space_pos);
}

consteval std::string_view make_impl_version() {
    std::string_view impl_version_str = ASMGRADER_IMPL_VERSION_INFO;

    std::size_t space_pos = impl_version_str.find(' ');

    if (space_pos == std::string_view::npos) {
        return "(version unknown)";
    }

    return impl_version_str.substr(space_pos + 1);
}

template <StaticString DepsStr>
consteval auto convert_deps_str() {
    // Consists of a number of space seperated "<libname> <version>" strings
    // Example: "fmt 11.1.3 spdlog 1.15.1" ...

    // As by spec above: count => number of spaces / 2 + 1
    constexpr auto num_deps = gsl::narrow<std::size_t>((ranges::count(DepsStr, ' ') / 2) + 1);

    std::array<buildinfo::Dependency, num_deps> result{};

    // many range-v3 views are not constexpr :(

    // raw loop because the stdlib 12 implementation for aarch64 cross compilation seems
    // to have bugs with considering std::string_view::find constexpr

    std::string_view deps_sv = DepsStr;
    std::size_t last_start = 0;
    auto res_iter = result.begin();
    bool found_name = false;

    for (std::size_t i = 0; i < deps_sv.size(); i++) {
        // Only do processing at ' '
        if (deps_sv.at(i) != ' ') {
            continue;
        }

        if (!found_name) {
            res_iter->name = deps_sv.substr(last_start, i - last_start);
        } else {
            res_iter->version = deps_sv.substr(last_start, i - last_start);
            ++res_iter;
        }

        found_name = !found_name;

        last_start = i + 1;
    }

    // set the last version field which was not processed
    result.back().version = deps_sv.substr(last_start);

    return result;
}

consteval BuildInfo make_build_info() {
    return BuildInfo{
        .major = ASMGRADER_VERSION_MAJOR,
        .minor = ASMGRADER_VERSION_MINOR,
        .patch = ASMGRADER_VERSION_PATCH,
        .git_hash = ASMGRADER_VERSION_GIT_HASH_STR,
        .build_type = ASMGRADER_VERSION_BUILD_TYPE_STR,
        .system_processor = SYSTEM_PROCESSOR,
        .endianness = EndiannessKind::Native,
        .compiler_info = make_compiler_info(),
        .app_mode = make_app_mode(),
        .impl_name = make_impl_name(),
        .impl_version = make_impl_version(),
        .cpp_standard = __cplusplus,
        .date = __DATE__,
        .time = __TIME__,
        .dependencies = static_deps_str
        //
    };
}

} // namespace detail

buildinfo::BuildInfo get_build_info() {
    return buildinfo::detail::make_build_info();
}

/// \returns std::array<buildinfo::Dependency, ...> for all of the dependencies found
/// in \ref get_build_info().dependencies
std::vector<Dependency> get_dependencies() {
    return detail::convert_deps_str<detail::static_deps_str>() | ranges::to<std::vector>();
}

const char* get_plain_version_str() {
    return BOOST_PP_STRINGIZE(ASMGRADER_VERSION_MAJOR) "." BOOST_PP_STRINGIZE(ASMGRADER_VERSION_MINOR) "." BOOST_PP_STRINGIZE(ASMGRADER_VERSION_PATCH);
}

// NOLINTBEGIN(readability-avoid-nested-conditional-operator)
// Adapted from https://stackoverflow.com/a/70567530
// Courtesy of Lundin
constexpr std::array date_iso8601 = std::to_array<char>({
    // YYYY year
    __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],

    // hyphen seperator
    '-',

    // First month letter, Oct Nov Dec = '1' otherwise '0'
    (__DATE__[0] == 'O' || __DATE__[0] == 'N' || __DATE__[0] == 'D') ? '1' : '0',

    // Second month letter
    (__DATE__[0] == 'J')   ? ((__DATE__[1] == 'a') ? '1' : // Jan, Jun or Jul
                                ((__DATE__[2] == 'n') ? '6' : '7'))
    : (__DATE__[0] == 'F') ? '2'
                           : // Feb
        (__DATE__[0] == 'M') ? (__DATE__[2] == 'r') ? '3' : '5'
                             : // Mar or May
        (__DATE__[0] == 'A') ? (__DATE__[1] == 'p') ? '4' : '8'
                             : // Apr or Aug
        (__DATE__[0] == 'S') ? '9'
                             : // Sep
        (__DATE__[0] == 'O') ? '0'
                             : // Oct
        (__DATE__[0] == 'N') ? '1'
                             : // Nov
        (__DATE__[0] == 'D') ? '2'
                             : // Dec
        0,

    // hyphen seperator
    '-',

    // First day number / space, replace space with digit
    __DATE__[4] == ' ' ? '0' : __DATE__[4],

    // Second day number
    __DATE__[5],

    '\0' // null terminate
});

// NOLINTEND(readability-avoid-nested-conditional-operator)

consteval std::string_view get_date_iso8601() {
    std::string_view str = {date_iso8601.begin(), date_iso8601.end()};
    // do not include '\0' in string_view
    str.remove_suffix(1);
    return str;
}

std::string get_version_str() {
    auto build_info = get_build_info();

    return fmt::format("AsmGrader v{}.{}.{}-g{:.12} ({})\n", build_info.major, build_info.minor, build_info.patch,
                       build_info.git_hash, build_info.app_mode) +
           fmt::format("[Implementation] {} v{}", build_info.impl_name, build_info.impl_version);
}

} // namespace asmgrader::buildinfo

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output
fmt::appender fmt::formatter<::asmgrader::buildinfo::Dependency>::format(const asmgrader::buildinfo::Dependency& from,
                                                                         format_context& ctx) const {
    if (is_debug_format) {
        return fmt::format_to(ctx.out(), "Dependency{{.name = {:?}, .version = {:?}}}", from.name, from.version);
    }
    return fmt::format_to(ctx.out(), "{} {}", from.name, from.version);
}

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output
fmt::appender fmt::formatter<asmgrader::buildinfo::CompilerInfo>::format(const asmgrader::buildinfo::CompilerInfo& from,
                                                                         format_context& ctx) const {
    if (is_debug_format) {
        return fmt::format_to(ctx.out(), "CompilerInfo{{.vendor = {:?}, .major = {}, .minor = {}, .patch = {}}}",
                              from.vendor, from.major, from.minor, from.patch);
    }
    return fmt::format_to(ctx.out(), "{} v{}.{}.{}", from.vendor, from.major, from.minor, from.patch);
}

/// "{}" spec gives a human-readable output; "{:?}" spec gives structured output

fmt::appender fmt::formatter<asmgrader::buildinfo::BuildInfo>::format(const asmgrader::buildinfo::BuildInfo& from,
                                                                      format_context& ctx) const {
    if (is_debug_format) {
        return fmt::format_to(
            ctx.out(),
            "BuildInfo{{.major = {}, .minor = {}, .patch = {}, .git_hash = {:?}, .build_type = {:?}, .system_processor "
            "= {}, .endianness = {}, .compiler_info = {:?}, .app_mode = {:?}, "
            ".impl_name = {:?}, .impl_version = {:?}, .cpp_standard = {}, .date = {:?} [iso8601 = {:?}], .time = "
            "{:?}, .dependencies = {:?} [processed = {::?}]}}",
            from.major, from.minor, from.patch, from.git_hash, from.build_type, from.system_processor, from.endianness,
            from.compiler_info, from.app_mode, from.impl_name, from.impl_version, from.cpp_standard, from.date,
            asmgrader::buildinfo::get_date_iso8601(), from.time, from.dependencies,
            asmgrader::buildinfo::get_dependencies());
    }

    // Only include first 12 chars from git hash
    ctx.advance_to(fmt::format_to(ctx.out(), "AsmGrader v{}.{}.{}-g{:.12} ({})\n", from.major, from.minor, from.patch,
                                  from.git_hash, from.app_mode));
    ctx.advance_to(fmt::format_to(ctx.out(), "[Implementation] {} v{}\n", from.impl_name, from.impl_version));
    *ctx.out()++ = '\n';
    ctx.advance_to(fmt::format_to(ctx.out(), "Build Type: {}\n", from.build_type));
    ctx.advance_to(fmt::format_to(ctx.out(), "Target: {}, {}\n", from.system_processor, from.endianness));
    ctx.advance_to(fmt::format_to(ctx.out(), "Compiler: {} (C++ {})\n", from.compiler_info, from.cpp_standard));
    *ctx.out()++ = '\n';
    ctx.advance_to(fmt::format_to(ctx.out(), "Dependencies:\n\t{}\n",
                                  fmt::join(asmgrader::buildinfo::get_dependencies(), "\n\t")));
    *ctx.out()++ = '\n';
    ctx.advance_to(fmt::format_to(ctx.out(), "Built: {} {} ({})\n", asmgrader::buildinfo::get_date_iso8601(), from.time,
                                  from.date));

    return ctx.out();
}
