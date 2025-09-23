#pragma once

#include <asmgrader/common/error_types.hpp>
#include <asmgrader/subprocess/run_result.hpp>
#include <asmgrader/subprocess/subprocess.hpp>
#include <asmgrader/subprocess/syscall_record.hpp>
#include <asmgrader/subprocess/tracer.hpp>

#include <chrono>
#include <functional>
#include <optional>

namespace asmgrader {

/// A subprocess managed by a tracer
class TracedSubprocess : public Subprocess
{
public:
    using Subprocess::Subprocess;

    ~TracedSubprocess() override;

    Tracer& get_tracer() { return tracer_; }

    const Tracer& get_tracer() const { return tracer_; }

    /// \see Tracer::run
    Result<RunResult> run();

    /// \see Tracer::run
    Result<RunResult> run_until(const std::function<bool(SyscallRecord)>& pred);

    /// Blocks until exit or timeout
    Result<int> wait_for_exit(std::chrono::microseconds timeout) override;

    std::optional<int> get_exit_code() const { return tracer_.get_exit_code(); }

private:
    Result<void> init_child() final;
    Result<void> init_parent() final;

    Tracer tracer_;
};

} // namespace asmgrader
