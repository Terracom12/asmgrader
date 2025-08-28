#include "subprocess/traced_subprocess.hpp"

#include "logging.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/subprocess.hpp"
#include "subprocess/tracer.hpp"

#include <chrono>

namespace asmgrader {

TracedSubprocess::~TracedSubprocess() {
    using namespace std::chrono_literals;

    std::ignore = close_pipes();

    // Give the child time to complete operations in case that open pipes were blocking
    std::this_thread::sleep_for(10ms);

    // Try running child process to exit one last time, as it may have been blocking on open pipes
    if (is_alive()) {
        std::ignore = run();
    }

    LOG_DEBUG("Processed {} syscalls", tracer_.get_records().size());

    if (is_alive()) {
        std::ignore = kill();
    }
}

Result<void> TracedSubprocess::init_child() {
    TRY(Subprocess::init_child());

    TRY(Tracer::init_child());

    return {};
}

Result<void> TracedSubprocess::init_parent() {
    TRY(Subprocess::init_parent());

    TRY(tracer_.begin(get_pid()));

    return {};
}

Result<RunResult> TracedSubprocess::run() {
    return tracer_.run();
}

Result<int> TracedSubprocess::wait_for_exit(std::chrono::microseconds /*timeout*/) {
    UNIMPLEMENTED("TracedSubprocess::wait_for_exit is not implemented; use TracedSubprocess::run instead.");
}

} // namespace asmgrader
