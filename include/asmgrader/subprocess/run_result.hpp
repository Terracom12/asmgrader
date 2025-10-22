#pragma once

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace asmgrader {

class RunResult
{
public:
    enum class Kind { Exited, Killed, SignalCaught };

    static constexpr RunResult make_exited(int code);
    static constexpr RunResult make_killed(int code);
    static constexpr RunResult make_signal_caught(int code);

    constexpr Kind get_kind() const;
    constexpr int get_code() const;

    constexpr bool operator==(const RunResult&) const = default;

private:
    constexpr RunResult(Kind kind, int code);

    Kind kind_;
    int code_;
};

constexpr RunResult::RunResult(Kind kind, int code)
    : kind_{kind}
    , code_{code} {}

constexpr RunResult RunResult::make_exited(int code) {
    return {Kind::Exited, code};
}

constexpr RunResult RunResult::make_killed(int code) {
    return {Kind::Killed, code};
}

constexpr RunResult RunResult::make_signal_caught(int code) {
    return {Kind::SignalCaught, code};
}

constexpr RunResult::Kind RunResult::get_kind() const {
    return kind_;
}

constexpr int RunResult::get_code() const {
    return code_;
}

static constexpr auto RUN_SUCCESS = RunResult::make_exited(0);

constexpr std::string_view format_as(const RunResult::Kind& from) {
    switch (from) {
    case RunResult::Kind::Exited:
        return "Exited";
    case RunResult::Kind::Killed:
        return "Killed";
    case RunResult::Kind::SignalCaught:
        return "SignalCaught";
    default:
        return "<unknown>";
    }
}

inline std::string format_as(const RunResult& from) {
    return fmt::format("{}({})", from.get_kind(), from.get_code());
}

} // namespace asmgrader
