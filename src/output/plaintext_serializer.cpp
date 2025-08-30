#include "output/plaintext_serializer.hpp"

#include "common/terminal_checks.hpp"
#include "common/time.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "output/serializer.hpp"
#include "output/sink.hpp"
#include "output/verbosity.hpp"
#include "user/program_options.hpp"
#include "version.hpp"

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <range/v3/algorithm/all_of.hpp>

#include <cctype>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>

#include <sys/ioctl.h>
#include <unistd.h>

namespace asmgrader {

using enum ProgramOptions::VerbosityLevel;

PlainTextSerializer::PlainTextSerializer(Sink& sink, ProgramOptions::ColorizeOpt colorize_option,
                                         ProgramOptions::VerbosityLevel verbosity)
    : Serializer{sink, verbosity}
    , do_colorize_{process_colorize_opt(colorize_option)}
    , terminal_width_{get_terminal_width()} {}

void PlainTextSerializer::on_requirement_result(const RequirementResult& data) {
    if (!should_output_requirement(verbosity_, data.passed)) {
        return;
    }

    std::string req_result_str;

    if (data.passed) {
        req_result_str = style_str("PASSED", SUCCESS_STYLE);
    } else {
        req_result_str = style_str("FAILED", ERROR_STYLE);
    }

    std::string out = fmt::format("Requirement {} : {}\n", req_result_str, style(data.msg, VALUE_STYLE));

    sink_.write(out);
}

void PlainTextSerializer::on_test_begin(std::string_view test_name) {
    if (!should_output_test(verbosity_)) {
        return;
    }

    std::string header_msg = fmt::format("{0}\nTest Case: {1}\n{0}\n", LINE_DIVIDER(terminal_width_), test_name);

    sink_.write(header_msg);
}

void PlainTextSerializer::on_test_result(const TestResult& data) {
    if (!should_output_test(verbosity_)) {
        return;
    }

    // Potentially output a msg for an empty test
    if (data.num_total == 0) {
        sink_.write("No test requirements.\n");
    }

    // Potentially output a msg hidden passing requirements
    if (data.num_passed > 0 && !should_output_requirement(verbosity_, /*passed=*/true)) {
        std::string hidden_reqs_msg =
            fmt::format("+ {} passing requirements.\n", style_str(data.num_passed, SUCCESS_STYLE, "{} hidden"));
        sink_.write(hidden_reqs_msg);
    }

    // Extra newline to seperate tests
    sink_.write("\n");
}

void PlainTextSerializer::on_assignment_result(const AssignmentResult& data) {
    if (should_output_grade_percentage(verbosity_)) {
        sink_.write("Output score ");
        output_grade_percentage(data);
        sink_.write("\n");
    }

    if (!should_output_student_summary(verbosity_)) {
        return;
    }

    std::string out;

    // We don't want to repeatedly output the same assignment name for every student in prof mode
    if (APP_MODE == AppMode::Student) {
        out = fmt::format("{0}\nAssignment: {1}\n{0}\n", LINE_DIVIDER_EM(terminal_width_), data.name);
    } else {
        out = LINE_DIVIDER(terminal_width_) + "\n";
    }

    auto labeled_num = [](int num, std::string_view label_singular) {
        return fmt::format("{} {}", num, pluralize(label_singular, num));
    };

    // Mostly copying Catch2's result summary format for now, so credit to them for the following

    if (data.all_passed()) {
        std::string success_msg = style_str("All tests passed", SUCCESS_STYLE);
        std::string requirements_msg = labeled_num(data.num_requirements_total, "requirement");
        std::string tests_msg = labeled_num(static_cast<int>(data.test_results.size()), "test");

        out += fmt::format("{} ({} in {})\n", success_msg, requirements_msg, tests_msg);
        sink_.write(out);

        return;
    }

    // We would need >99999 requirements for this to look off
    static constexpr std::size_t field_width = 12;

    auto pass_fail_line = [this](std::string_view label, int num_passed, int num_failed) {
        int num_total = num_passed + num_failed;

        std::string total_msg = fmt::format("{} total", num_total);
        std::string passed_msg = fmt::format("{} passed", num_passed);
        std::string failed_msg = fmt::format("{} failed", num_failed);

        return fmt::format("{0:<{4}}: {1:>{4}} | {2:>{4}} | {3:>{4}}", label, total_msg,
                           style(passed_msg, SUCCESS_STYLE), style(failed_msg, ERROR_STYLE), field_width);
    };

    std::string tests_line = pass_fail_line("Tests", data.num_tests_passed(), data.num_tests_failed());
    std::string requirements_line =
        pass_fail_line("Requirements", data.num_requirements_passed(), data.num_requirements_failed());

    out += fmt::format("{}\n{}\n", tests_line, requirements_line);

    // Extra line
    if (APP_MODE == AppMode::Professor) {
    }

    sink_.write(out);
}

void PlainTextSerializer::finalize() {}

bool PlainTextSerializer::process_colorize_opt(ProgramOptions::ColorizeOpt colorize_option) {
    using enum ProgramOptions::ColorizeOpt;

    if (colorize_option == Never) {
        return false;
    }
    if (colorize_option == Always) {
        return true;
    }

    // Colorize if output is going to a color-supporting terminal, otherwise do not
    LOG_DEBUG("In terminal: {} & Color Supporting Terminal: {}", in_terminal(stdout), is_color_terminal());

    return in_terminal(stdout) && is_color_terminal();
}

std::string PlainTextSerializer::pluralize(std::string_view root, int count, std::string_view suffix,
                                           std::size_t replace_last_chars) {
    if (count == 1) {
        return std::string{root};
    }

    root.remove_suffix(replace_last_chars);

    return fmt::format("{}{}", root, suffix);
}

void PlainTextSerializer::on_student_begin(const StudentInfo& info) {

    static bool is_first = true;

    is_first = false;

    std::string name_text;

    if (info.names_known) {
        name_text = info.first_name + " " + info.last_name;
    } else {
        name_text = "<unknown>";

        if (!info.first_name.empty()) {
            name_text += fmt::format(" (\"{}\" inferred)", info.first_name);
        }
    }

    // If we want short form, just output student name with no newline as to have output of the form:
    // NAME: Output score ...
    if (!should_output_student_summary(verbosity_)) {
        std::string out = fmt::format("{}: ", styled(name_text, POP_OUT_STYLE));

        // TODO: DRY
        if (!info.assignment_path) {
            out += style_str("Could not locate executable", ERROR_STYLE);
            out += "\n";
        }

        sink_.write(out);

        return;
    }

    std::string student_label = "Student: ";
    std::string student_label_text = student_label + style_str(name_text, POP_OUT_STYLE);

    const auto actual_sz = student_label.size() + name_text.size();

    std::string out = fmt::format("{}\n{:>{}}\n\n", LINE_DIVIDER_EM(terminal_width_), student_label_text,
                                  student_label_text.size() + ((terminal_width_ - actual_sz) / 2));

    if (info.assignment_path) {
        out += info.assignment_path->relative_path().string() + "\n";
    } else {
        std::string regex_text;

        if (ranges::all_of(info.subst_regex_string, [](unsigned char chr) { return std::isprint(chr); })) {
            regex_text = fmt::format("\"{}\"", info.subst_regex_string);
        } else {
            regex_text = fmt::format("{:?}", info.subst_regex_string);
        }

        out += fmt::format(ERROR_STYLE, "Could not locate executable [RegEx matcher: {}]\n", regex_text);
    }

    // 4 newlines to separate each student
    if (!is_first) {
        sink_.write("\n\n\n\n");
    }
    sink_.write(out);
}

void PlainTextSerializer::on_student_end([[maybe_unused]] const StudentInfo& info) {
    if (!should_output_student_summary(verbosity_)) {
        return;
    }

    // Line divider at the end to make it easier to differentiate between students
    std::string out = LINE_DIVIDER_EM(terminal_width_) + "\n";
    sink_.write(out);
}

void PlainTextSerializer::on_warning(std::string_view what) {
    std::string out = style_str(what, WARNING_STYLE, "{}\n");
    sink_.write(out);
}

void PlainTextSerializer::on_error(std::string_view what) {
    std::string out = style_str(what, ERROR_STYLE, "{}\n");
    sink_.write(out);
}

void PlainTextSerializer::on_run_metadata(const RunMetadata& data) {
    constexpr std::string_view header_text = "Execution Info";
    constexpr std::string_view version_label = "Version: ";
    constexpr std::string_view date_label = "Date and Time: ";

    std::string version_text = fmt::format("{}-g{}", data.version_string, data.git_hash);

    if (APP_MODE == AppMode::Professor) {
        version_text += " (Professor)";
    } else {
        version_text += " (Student)";
    }

    std::string local_timepoint_text =
        asmgrader::to_localtime_string(data.start_time, "%a %b %d %T %Y").value_or("<ERROR>");

    std::string out = fmt::format("{:#^{}}\n", header_text, terminal_width_);
    out += fmt::format("{}{:>{}}\n", version_label, version_text, terminal_width_ - version_label.size());
    out += fmt::format("{:}{:>{}}\n", date_label, local_timepoint_text, terminal_width_ - date_label.size());
    out += LINE_DIVIDER_2EM(terminal_width_) + "\n\n";

    sink_.write(out);
}

std::size_t PlainTextSerializer::get_terminal_width() {
    auto width = terminal_size(stdout).transform([](const winsize& size) { return size.ws_col; });

    if (width.has_error()) {
        LOG_WARN("Could not obtain terminal width because {}. Defaulting to {}", width.error(), DEFAULT_WIDTH);
    }

    return width.value_or(DEFAULT_WIDTH);
}

void PlainTextSerializer::output_grade_percentage(const AssignmentResult& data) {
    std::string out = fmt::format("{:.2f}% ({}/{} points)", data.get_percentage(), data.num_requirements_passed(),
                                  data.num_requirements_total);

    sink_.write(out);
}

} // namespace asmgrader
