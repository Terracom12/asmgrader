#include "catch2_custom.hpp"

#include "logging.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/subprocess.hpp"
#include "subprocess/syscall_record.hpp"
#include "subprocess/traced_subprocess.hpp"
#include "common/error_types.hpp"
#include "common/timespec_operator_eq.hpp"

#include <fmt/ranges.h>

#include <chrono>
#include <ctime>
#include <variant>

#include <sys/syscall.h>
#include <unistd.h>

// static_assert(requires(util::Result<std::timespec> r, std::timespec t) {
//     t == t;
//     r == r;
//     r == t;
//     t == r;
// });
// static_assert(requires(std::variant<util::Result<std::timespec>, double> a, util::Result<std::timespec> r) {
//     a == a;
//     a != a;
//     a == r;
//     3.14 == a;
// });

TEST_CASE("Read /bin/echo stdout") {
    using namespace std::chrono_literals;
    Subprocess proc("/bin/echo", {"-n", "Hello", "world!"});
    proc.start();

    proc.wait_for_exit();

    REQUIRE(proc.read_stdout() == "Hello world!");
}

TEST_CASE("Interact with /bin/cat") {
    using namespace std::chrono_literals;

    Subprocess proc("/bin/cat", {});
    proc.start();

    proc.send_stdin("Goodbye dog...");

    REQUIRE(proc.read_stdout(100ms) == "Goodbye dog...");

    proc.send_stdin("Du Du DUHHH");

    REQUIRE(proc.read_stdout(100ms) == "Du Du DUHHH");
}

TEST_CASE("Get results of asm program") {
    TracedSubprocess proc(ASM_TESTS_EXEC, {});
    proc.start();

    auto run_res = proc.run();

    REQUIRE(run_res->get_kind() == RunResult::Kind::Exited);
    REQUIRE(run_res->get_code() == 42);

    REQUIRE(proc.get_exit_code() == 42);
    REQUIRE(proc.read_stdout() == "Hello, from assembly!\n");

    auto syscall_records = proc.get_tracer().get_records();

    LOG_DEBUG("Syscall records:\n\t{:?}", fmt::join(syscall_records, "\n\t"));

    REQUIRE(syscall_records.size() == 2);
    // REQUIRE(syscall_records.at(0).num == SYS_write);
    // REQUIRE(syscall_records.at(1).num == SYS_exit);
    // REQUIRE(syscall_records.at(1).args.size() == 1);
    // REQUIRE(syscall_records.at(1).args.at(0) == SyscallRecord::SyscallArg{42});
}
