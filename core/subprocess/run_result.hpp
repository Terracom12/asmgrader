#pragma once

namespace asmgrader {

class RunResult
{
public:
    enum class Kind { Exited, Killed, SignalCaught };

    static RunResult make_exited(int code);
    static RunResult make_killed(int code);
    static RunResult make_signal_caught(int code);

    Kind get_kind() const;
    int get_code() const;

private:
    RunResult(Kind kind, int code);

    Kind kind_;
    int code_;
};

} // namespace asmgrader
