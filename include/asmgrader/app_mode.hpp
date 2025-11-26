#pragma once

#include <string_view>

namespace asmgrader {

enum AppMode { Student, Professor };

consteval AppMode get_builtin_app_mode() {
#ifdef PROFESSOR_VERSION
    return AppMode::Professor;
#else
    return AppMode::Student;
#endif
}

// TODO: mode setting for prof
consteval bool is_student_mode() {
    return get_builtin_app_mode() == AppMode::Student;
}

// TODO: mode setting for prof
consteval bool is_prof_mode() {
    return get_builtin_app_mode() == AppMode::Professor;
}

constexpr std::string_view format_as(const AppMode& from) {
    using enum AppMode;
    switch (from) {
    case Student:
        return "Student";
    case Professor:
        return "Professor";
    default:
        return "<unknown>";
    }
};

} // namespace asmgrader
