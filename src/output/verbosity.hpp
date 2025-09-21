#pragma once

#include "user/program_options.hpp"
#include "version.hpp"

namespace asmgrader {

/// See \ref VerbosityLevel
constexpr bool should_output_requirement(ProgramOptions::VerbosityLevel level, bool passed) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student &&                          //
            ((level >= Summary) || (level >= FailsOnly && !passed))) //
           ||                                                        //
           (APP_MODE == AppMode::Professor &&                        //
            ((level >= All) || (level >= FailsOnly && !passed)));    //
}

/// See \ref VerbosityLevel
constexpr bool should_output_test(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student && level >= Summary)      //
           ||                                                      //
           (APP_MODE == AppMode::Professor && level >= FailsOnly); //
}

/// See \ref VerbosityLevel
constexpr bool should_output_student_summary(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student && level >= Quiet)      //
           ||                                                    //
           (APP_MODE == AppMode::Professor && level >= Summary); //
}

/// See \ref VerbosityLevel
constexpr bool should_output_grade_percentage(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Professor && level >= Quiet);
}

/// See \ref VerbosityLevel
constexpr bool should_output_requirement_details(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (level >= Extra);
}

/// See \ref VerbosityLevel
constexpr bool should_output_run_metadata(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return level > Silent;
}

} // namespace asmgrader
