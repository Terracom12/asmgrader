#pragma once

#include "grading_session.hpp"
#include "output/serializer.hpp"
#include "output/sink.hpp"
#include "user/program_options.hpp"

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>

#include <string>

class PlainTextSerializer : public Serializer
{
public:
    PlainTextSerializer(Sink& sink, ProgramOptions::ColorizeOpt colorize_option, bool verbose);

    void on_requirement_result(const RequirementResult& data) override;
    void on_test_result(const TestResult& data) override;
    void on_assignment_result(const AssignmentResult& data) override;
    void on_metadata() override;

    void finalize() override;

private:
    static bool process_colorize_opt(ProgramOptions::ColorizeOpt colorize_option);

    template <fmt::formattable T>
    std::string style(const T& arg, fmt::text_style style, fmt::format_string<T> fmt = "{}") const;

    // Basic styles for different kinds of output:
    //   error    - FAILED messages, fatal errors, etc.
    //   success  - PASSED messages
    //   header   - section header (like "String Test Results:")
    //   value    - for variables or literal values referenced for test requirements
    static constexpr auto ERROR_STYLE = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
    static constexpr auto SUCCESS_STYLE = fmt::fg(fmt::color::lime_green);
    static constexpr auto HEADER_STYLE = fmt::emphasis::underline | fmt::emphasis::bold;
    static constexpr auto VALUE_STYLE = fmt::fg(fmt::color::aqua);

    bool do_colorize_;
    bool verbose_;
};

template <fmt::formattable T>
std::string PlainTextSerializer::style(const T& arg, fmt::text_style style, fmt::format_string<T> fmt) const {
    if (!do_colorize_) {
        return fmt::vformat(fmt.str, fmt::vargs<T>{arg});
    }

    return fmt::vformat(style, fmt.str, fmt::vargs<T>{arg});
}
