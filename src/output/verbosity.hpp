#pragma once

#include "user/program_options.hpp"
#include "version.hpp"

namespace asmgrader {

/// VerbosityLevel Meanings:
///  - Professor mode is generally one level "quieter" than student mode for the output of each individual student
///  - Each level includes all output from levels below, possibly formatted a little differently
///    e.g., Setting the verbosity to `FailedOnly` results in all output described in:
///         `FailedOnly`, `Summary`, and `Quiet` (though not the retcode from `Silent`)
///
/// Silent
///   (Useful for scripting)
///   Student - Nothing on stdout. Return code = # of failed tests
///   Prof    - Nothing on stdout. Return code = # of students not passing all tests
///
/// Quiet
///   Student - Overall test results only (# of tests and requirements passed/failed)
///   Prof    - Overall statistics only (# of students passed/failed, etc.)
///
/// Summary (default)
///   Student - Lists the names of all tests, but only failing requirements
///   Prof    - Pass / fail stats for each individual student (like `Quiet` in student mode)
///
/// FailsOnly
///   Student - Same as `Summary` for now
///   Prof    - Lists the names of all tests, but only failing requirements for each individual student
///
/// All
///   Student - Lists the result of all tests and requirements
///   Prof    - """""""""""""""""""""""""""""""""""""""""""""" for each individual student
///
/// Extra
///   (Not implemented yet)
///   Student - ...
///   Prof    - ...
///
/// Max
///   Student - Same as `Extra` for now
///   Prof    - """""""""""""""""""""""

constexpr bool should_output_requirement(ProgramOptions::VerbosityLevel level, bool passed) {
    using enum ProgramOptions::VerbosityLevel;

    return (APP_MODE == AppMode::Student &&                         //
            ((level >= Summary) || (level >= FailsOnly && passed))) //
           ||                                                       //
           (APP_MODE == AppMode::Professor &&                       //
            ((level >= All) || (level >= FailsOnly && passed)));    //
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

} // namespace asmgrader
