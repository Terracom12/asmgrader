#pragma once

#include "subprocess/run_result.hpp"
#include "subprocess/subprocess.hpp"
#include "subprocess/tracer.hpp"
#include "util/error_types.hpp"

#include <chrono>

/// A subprocess managed by a tracer
class TracedSubprocess : public Subprocess
{
public:
    using Subprocess::Subprocess;

    ~TracedSubprocess() override;

    Tracer& get_tracer() { return tracer_; }
    const Tracer& get_tracer() const { return tracer_; }

    /// Run until timeout or process exit
    util::Result<RunResult> run();

    /// Blocks until exit or timeout
    util::Result<int> wait_for_exit(std::chrono::microseconds timeout) override;

    std::optional<int> get_exit_code() const { return tracer_.get_exit_code(); }

private:
    util::Result<void> init_child() final;
    util::Result<void> init_parent() final;

    Tracer tracer_;
};
