#include "output/plaintext_serializer.hpp"

#include "grading_session.hpp"
#include "output/serializer.hpp"
#include "output/sink.hpp"
#include "user/program_options.hpp"

#include <fmt/color.h>
#include <fmt/format.h>

#include <string>

#include <unistd.h>

PlainTextSerializer::PlainTextSerializer(Sink& sink, ProgramOptions::ColorizeOpt colorize_option, bool verbose)
    : Serializer{sink}
    , do_colorize_{process_colorize_opt(colorize_option)}
    , verbose_{verbose} {}

void PlainTextSerializer::on_requirement_result(const RequirementResult& data) {
    // Don't output successful requirements if not in verbose mode
    if (data.passed && !verbose_) {
        return;
    }

    std::string req_result_str;

    if (data.passed) {
        req_result_str = style("PASSED", SUCCESS_STYLE);
    } else {
        req_result_str = style("FAILED", ERROR_STYLE);
    }

    std::string msg_str = style(data.msg, VALUE_STYLE);

    std::string out = fmt::format("Requirement {} : {}\n", req_result_str, msg_str);

    sink_.write(out);
}

void PlainTextSerializer::on_test_result(const TestResult& data) {
    for (const RequirementResult& req : data.requirement_results) {
        on_requirement_result(req);
    }
}

void PlainTextSerializer::on_assignment_result([[maybe_unused]] const AssignmentResult& data) {}

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

    // Colorize if output is going to a terminal, otherwise do not
    return ::isatty(STDOUT_FILENO) == 1;
}
