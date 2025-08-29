#pragma once

#include "user/program_options.hpp"
#include "version.hpp"

namespace asmgrader {

constexpr bool should_output_requirement(ProgramOptions::VerbosityLevel level, bool passed) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student &&                          //
            ((level >= Summary) || (level >= FailsOnly && !passed))) //
           ||                                                        //
           (APP_MODE == AppMode::Professor &&                        //
            ((level >= All) || (level >= FailsOnly && !passed)));    //
}

constexpr bool should_output_test(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student && level >= Summary)      //
           ||                                                      //
           (APP_MODE == AppMode::Professor && level >= FailsOnly); //
}

constexpr bool should_output_student_summary(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student && level >= Quiet)      //
           ||                                                    //
           (APP_MODE == AppMode::Professor && level >= Summary); //
}

constexpr bool should_output_grade_percentage(ProgramOptions::VerbosityLevel level) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Professor && level >= Quiet);
}

} // namespace asmgrader
