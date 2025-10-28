#include "app/test_theme_app.hpp"

#include "api/expression_inspection.hpp"
#include "api/syntax_highlighter.hpp"
#include "common/terminal_checks.hpp"

#include <fmt/base.h>
#include <fmt/color.h>

#include <exception>
#include <iostream>
#include <string>

namespace asmgrader {

int TestThemeApp::run_impl() {
    if (!is_color_terminal()) {
        fmt::print(stderr, fg(fmt::color::red), "Not in a color terminal. Exiting.\n");
        return 1;
    }

    if (!in_terminal(stdin)) {
        fmt::print(stderr, fg(fmt::color::yellow), "stdin must come from a terminal. Exiting.\n");
        return 2;
    }

    fmt::print(stderr, fg(fmt::color::alice_blue), "Highlighting each line of input. Type Ctrl+D to exit.\n\n");

    std::string line;
    fmt::print("Input: ");
    while (std::getline(std::cin, line)) {
        try {
            std::string resolved_blocks = highlight::render_blocks(line, /*skip_styling=*/false);
            std::string resolved_blocks_noformatting = highlight::render_blocks(line, /*skip_styling=*/true);
            if (resolved_blocks != line) {
                fmt::print("Resolved blocks: {}\n", resolved_blocks);
            }
            auto tokens = inspection::Tokenizer<>(resolved_blocks_noformatting);
            fmt::print("Tokens: {}\n", tokens);
            std::string highlighted = highlight::highlight(tokens);
            fmt::print("Highlighted: \"{}\"\n\n", highlighted);
        } catch (std::exception& ex) {
            fmt::print(stderr, fg(fmt::color::red), "Error parsing input: {}\n", ex.what());
        }
        fmt::print("Input: ");
    }

    return 0;
}

} // namespace asmgrader
