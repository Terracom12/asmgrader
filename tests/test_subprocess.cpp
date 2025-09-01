#include "catch2_custom.hpp"

#include "common/error_types.hpp"
#include "common/timespec_operator_eq.hpp" // IWYU pragma: keep
#include "logging.hpp"
#include "subprocess/run_result.hpp"
#include "subprocess/subprocess.hpp"
#include "subprocess/syscall_record.hpp"
#include "subprocess/traced_subprocess.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/ranges.h>

#include <chrono>
#include <ctime>
#include <variant>

#include <sys/syscall.h>
#include <unistd.h>

TEST_CASE("timespec operator== is defined correctly") {
    STATIC_REQUIRE(requires(asmgrader::Result<std::timespec> r, std::timespec t) {
        t == t;
        r == r;
        r == t;
        t == r;
    });

    STATIC_REQUIRE(
        requires(std::variant<asmgrader::Result<std::timespec>, double> a, asmgrader::Result<std::timespec> r) {
            a == a;
            a != a;
        });
}

TEST_CASE("Read /bin/echo stdout") {
    using namespace std::chrono_literals;
    asmgrader::Subprocess proc("/bin/echo", {"-n", "Hello", "world!"});
    REQUIRE(proc.start());

    proc.wait_for_exit();

    REQUIRE(proc.read_stdout() == "Hello world!");
}

TEST_CASE("Interact with /bin/cat") {
    using namespace std::chrono_literals;

    asmgrader::Subprocess proc("/bin/cat", {});
    REQUIRE(proc.start());

    REQUIRE(proc.send_stdin("Goodbye dog..."));

    REQUIRE(proc.read_stdout(100ms) == "Goodbye dog...");

    REQUIRE(proc.send_stdin("Du Du DUHHH"));

    REQUIRE(proc.read_stdout(100ms) == "Du Du DUHHH");
}

TEST_CASE("Get results of asm program") {
    asmgrader::TracedSubprocess proc(ASM_TESTS_EXEC, {});
    REQUIRE(proc.start());

    auto run_res = proc.run();

    REQUIRE(run_res->get_kind() == asmgrader::RunResult::Kind::Exited);
    REQUIRE(run_res->get_code() == 42);

    REQUIRE(proc.get_exit_code() == 42);
    REQUIRE(proc.read_stdout() == "Hello, from assembly!\n");

    auto syscall_records = proc.get_tracer().get_records();

    LOG_DEBUG("Syscall records:\n\t{:?}", fmt::join(syscall_records, "\n\t"));

    REQUIRE(syscall_records.size() == 2);
    REQUIRE(syscall_records.at(0).num == SYS_write);
    REQUIRE(syscall_records.at(1).num == SYS_exit);
    REQUIRE(syscall_records.at(1).args.size() == 1);
    REQUIRE(syscall_records.at(1).args.at(0) == asmgrader::SyscallRecord::SyscallArg{42});
}
