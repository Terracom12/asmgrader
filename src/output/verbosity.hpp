#pragma once

#include "version.hpp"

namespace asmgrader {

/// See \ref verbosity_levels_desc for an explaination of each of the levels.
/// `Max` is just used as a sentinal for now
enum class VerbosityLevel {
    Silent,  ///< \ref verbosity_levels_silent_desc
    Quiet,   ///< \ref verbosity_levels_quiet_desc
    Summary, ///< \ref verbosity_levels_summary_desc
    All,     ///< \ref verbosity_levels_all_desc
    Extra,   ///< \ref verbosity_levels_extra_desc
    Max      ///< \ref verbosity_levels_max_desc
};

/// See \ref VerbosityLevel
constexpr bool should_output_requirement(VerbosityLevel level, bool passed) {
    using enum VerbosityLevel;

    return (level >= All || (level >= Summary && !passed));
}

/// See \ref VerbosityLevel
constexpr bool should_output_test(VerbosityLevel level) {
    using enum VerbosityLevel;

    return (level >= Summary);
}

/// See \ref VerbosityLevel
constexpr bool should_output_student_summary(VerbosityLevel level) {
    using enum VerbosityLevel;

    return (APP_MODE == AppMode::Student && level >= Quiet)      //
           ||                                                    //
           (APP_MODE == AppMode::Professor && level >= Summary); //
}

/// See \ref VerbosityLevel
constexpr bool should_output_grade_percentage(VerbosityLevel level) {
    using enum VerbosityLevel;

    return (APP_MODE == AppMode::Professor && level >= Quiet);
}

/// See \ref VerbosityLevel
constexpr bool should_output_requirement_details(VerbosityLevel level) {
    using enum VerbosityLevel;

    return (level >= All);
}

/// See \ref VerbosityLevel
constexpr bool should_output_run_metadata(VerbosityLevel level) {
    using enum VerbosityLevel;

    return (level > Silent);
}

} // namespace asmgrader
