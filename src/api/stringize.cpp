#include "api/stringize.hpp"

#include "api/syntax_highlighter.hpp"

#include <string>

namespace asmgrader::stringize {

std::string StringizeResult::resolve_blocks(bool do_colorize) const {
    return highlight::render_blocks(original, /*skip_styling=*/!do_colorize);
}

std::string StringizeResult::syntax_highlight() const {
    return highlight::highlight(original);
}

} // namespace asmgrader::stringize
