#include "output/plaintext_serializer.hpp"

#include "common/terminal_checks.hpp"
#include "grading_session.hpp"
#include "logging.hpp"
#include "output/serializer.hpp"
#include "output/sink.hpp"
#include "output/verbosity.hpp"
#include "user/program_options.hpp"

#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>

#include <unistd.h>

using enum ProgramOptions::VerbosityLevel;

PlainTextSerializer::PlainTextSerializer(Sink& sink, ProgramOptions::ColorizeOpt colorize_option,
                                         ProgramOptions::VerbosityLevel verbosity)
    : Serializer{sink, verbosity}
    , do_colorize_{process_colorize_opt(colorize_option)} {}

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

    std::string header_msg =
        fmt::format("{0}\nTest Case: {1}\n{0}\n", LINE_DIVIDER(LINE_DIVIDER_DEFAULT_WIDTH), test_name);

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
        std::string hidden_reqs_msg = fmt::format("{} hidden passing requirements.\n", data.num_passed);
        sink_.write(hidden_reqs_msg);
    }

    // Extra newline to seperate tests
    sink_.write("\n");
}

void PlainTextSerializer::on_assignment_result(const AssignmentResult& data) {
    if (!should_output_student_summary(verbosity_)) {
        return;
    }

    std::string out =
        fmt::format("{0}\nAssignment: {1}\n{0}\n", LINE_DIVIDER_EM(LINE_DIVIDER_DEFAULT_WIDTH), data.name);

    auto labeled_num = [](int num, std::string_view label_singular) {
        return fmt::format("{} {}", num, pluralize(label_singular, num));
    };

    // Mostly copying Catch2's format for now, so full credit to them

    if (data.all_passed()) {
        std::string success_msg = style_str("All tests passed", SUCCESS_STYLE);
        std::string requirements_msg = labeled_num(data.num_requirements_total, "requirement");
        std::string tests_msg = labeled_num(static_cast<int>(data.test_results.size()), "test");

        out += fmt::format("{} ({} in {})\n", success_msg, requirements_msg, tests_msg);
        sink_.write(out);

        return;
    }

    // We would need >99999 requirements for this to look off
    static constexpr std::size_t FIELD_WIDTH = 12;

    auto pass_fail_line = [this](std::string_view label, int num_passed, int num_failed) {
        int num_total = num_passed + num_failed;

        std::string total_msg = fmt::format("{} total", num_total);
        std::string passed_msg = fmt::format("{} passed", num_passed);
        std::string failed_msg = fmt::format("{} failed", num_failed);

        return fmt::format("{0:<{4}}: {1:>{4}} | {2:>{4}} | {3:>{4}}", label, total_msg,
                           style(passed_msg, SUCCESS_STYLE), style(failed_msg, ERROR_STYLE), FIELD_WIDTH);
    };

    std::string tests_line = pass_fail_line("Tests", data.num_tests_passed(), data.num_tests_failed());
    std::string requirements_line =
        pass_fail_line("Requirements", data.num_requirements_passed(), data.num_requirements_failed());

    out += fmt::format("{}\n{}\n", tests_line, requirements_line);
    sink_.write(out);
}

void PlainTextSerializer::on_metadata() {}

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
