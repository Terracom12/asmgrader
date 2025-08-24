#pragma once

#include "common/error_types.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/subprocess.hpp"
#include "subprocess/tracer.hpp"

#include <chrono>

namespace asmgrader {

/// A subprocess managed by a tracer
class TracedSubprocess : public Subprocess
{
public:
    using Subprocess::Subprocess;

    ~TracedSubprocess() override;

    Tracer& get_tracer() { return tracer_; }

    const Tracer& get_tracer() const { return tracer_; }

    /// Run until timeout or process exit
    Result<RunResult> run();

    /// Blocks until exit or timeout
    Result<int> wait_for_exit(std::chrono::microseconds timeout) override;

    std::optional<int> get_exit_code() const { return tracer_.get_exit_code(); }

private:
    Result<void> init_child() final;
    Result<void> init_parent() final;

    Tracer tracer_;
};

} // namespace asmgrader
