#include "run_result.hpp"

namespace asmgrader {

RunResult::RunResult(Kind kind, int code)
    : kind_{kind}
    , code_{code} {}

RunResult RunResult::make_exited(int code) {
    return {Kind::Exited, code};
}

RunResult RunResult::make_killed(int code) {
    return {Kind::Killed, code};
}

RunResult RunResult::make_signal_caught(int code) {
    return {Kind::SignalCaught, code};
}

RunResult::Kind RunResult::get_kind() const {
    return kind_;
}

int RunResult::get_code() const {
    return code_;
}

} // namespace asmgrader
