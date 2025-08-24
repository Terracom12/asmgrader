#pragma once

#include "grading_session.hpp"
#include "output/serializer.hpp"
#include "output/sink.hpp"
#include "user/program_options.hpp"

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>

class PlainTextSerializer : public Serializer
{
public:
    PlainTextSerializer(Sink& sink, ProgramOptions::ColorizeOpt colorize_option,
                        ProgramOptions::VerbosityLevel verbosity);

    void on_requirement_result(const RequirementResult& data) override;
    void on_test_begin(std::string_view test_name) override;
    void on_test_result(const TestResult& data) override;
    void on_assignment_result(const AssignmentResult& data) override;
    void on_metadata() override;

    void finalize() override;

private:
    static bool process_colorize_opt(ProgramOptions::ColorizeOpt colorize_option);

    template <fmt::formattable T>
    auto style(const T& arg, fmt::text_style style) const -> decltype(fmt::styled(arg, style));

    template <fmt::formattable T>
    std::string style_str(const T& arg, fmt::text_style style, fmt::format_string<T> fmt = "{}") const;

    /// Conditionally make a word singular or plural based on `count`
    /// Plural if and only if `count == 1`
    ///
    /// Examples:
    ///  pluralize("test", 0) => "tests"
    ///  pluralize("color", 1) => "color"
    ///  pluralize("datum", 42, "a", 2) => "data"
    ///  pluralize("datum", 1, "a", 2) => "datum"
    static std::string pluralize(std::string_view root, int count, std::string_view suffix = "s",
                                 std::size_t replace_last_chars = 0);

    // Basic styles for different kinds of output:
    //   error    - FAILED messages, fatal errors, etc.
    //   success  - PASSED messages
    //   header   - section header (like "String Test Results:")
    //   value    - for variables or literal values referenced for test requirements
    static constexpr auto ERROR_STYLE = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
    static constexpr auto SUCCESS_STYLE = fmt::fg(fmt::color::lime_green);
    static constexpr auto HEADER_STYLE = fmt::emphasis::underline | fmt::emphasis::bold;
    static constexpr auto VALUE_STYLE = fmt::fg(fmt::color::aqua);

    static constexpr std::size_t LINE_DIVIDER_DEFAULT_WIDTH = 64;

    static constexpr auto MAKE_LINE_DIVIDER = [](char chr) {
        return [chr](std::size_t len) { return std::string(len, chr); };
    };

    // Basic line dividers to seperate output, parameterized on length
    // Line Divider Emphasized : "======="...
    // Line Divider            : "--------...
    static const inline auto LINE_DIVIDER_EM = MAKE_LINE_DIVIDER('=');
    static const inline auto LINE_DIVIDER = MAKE_LINE_DIVIDER('-');

    bool do_colorize_;
};

template <fmt::formattable T>
auto PlainTextSerializer::style(const T& arg, fmt::text_style style) const -> decltype(fmt::styled(arg, style)) {
    if (!do_colorize_) {
        return fmt::styled(arg, {});
    }
    return fmt::styled(arg, style);
}

template <fmt::formattable T>
std::string PlainTextSerializer::style_str(const T& arg, fmt::text_style style, fmt::format_string<T> fmt) const {
    if (!do_colorize_) {
        return fmt::vformat(fmt.str, fmt::vargs<T>{arg});
    }

    return fmt::vformat(style, fmt.str, fmt::vargs<T>{arg});
}
