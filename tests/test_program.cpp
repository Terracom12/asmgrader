#include "catch2_custom.hpp"

#include "common/aliases.hpp"
#include "common/error_types.hpp"
#include "program/program.hpp"

#include <cstdint>
#include <string>

using namespace asmgrader::aliases;

using sum = u64(std::uint64_t, std::uint64_t);
using sum_and_write = void(u64, std::uint64_t);
using timeout_fn = void();
using segfaulting_fn = void();
using exiting_fn = void(u64);

TEST_CASE("Try to call functions that don't exist") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});

    using asmgrader::ErrorKind::UnresolvedSymbol;

    REQUIRE(prog.call_function<void(void)>("") == UnresolvedSymbol);

    REQUIRE(prog.call_function<void(void)>("abc123") == UnresolvedSymbol);

    REQUIRE(prog.call_function<void(void)>("_sum") == UnresolvedSymbol);
}

TEST_CASE("Call sum function") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});
    asmgrader::Result<u64> res;

    res = prog.call_function<sum>("sum", 0, 0);
    REQUIRE(res == 0ull);

    res = prog.call_function<sum>("sum", 1, 2);
    REQUIRE(res == 3ull);

    // unsigned, but overflow with 2's complement gives values as we would expect
    res = prog.call_function<sum>("sum", -1, -12);
    REQUIRE(res == static_cast<u64>(-13));
}

TEST_CASE("Call sum_and_write function") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});

    REQUIRE(prog.call_function<sum_and_write>("sum_and_write", 0, 0));
    REQUIRE(prog.get_subproc().read_stdout() == std::string{"\0\0\0\0\0\0\0\0", 8});

    REQUIRE(prog.call_function<sum_and_write>("sum_and_write", 'a', 5));
    // 'a' + 5 = 'f'
    REQUIRE(prog.get_subproc().read_stdout() == std::string{"f\0\0\0\0\0\0\0", 8});

    REQUIRE(prog.call_function<sum_and_write>("sum_and_write", 0x1010101010101010, 0x1010101010101010));
    static_assert(' ' == 0x10 + 0x10, "Somehow not ASCII encoded???");
    // 0x10 + 0x10 = 0x20 (space ' ')
    REQUIRE(prog.get_subproc().read_stdout() == "        ");
}

TEST_CASE("Test that timeouts are handled properly with timeout_fn") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});

    using asmgrader::ErrorKind::TimedOut;

    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // and again...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // ensure that calling other functions still works
    REQUIRE(prog.call_function<sum>("sum", 128, 42) == 170ull);

    // and one mre time for good measure...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);
}

TEST_CASE("Test that segfaults are essentially ignored") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});

    using enum asmgrader::ErrorKind;

    REQUIRE(prog.call_function<segfaulting_fn>("segfaulting_fn") == UnexpectedReturn);

    // ensure that calling other functions still works
    REQUIRE(prog.call_function<sum>("sum", 128, 42) == 170ull);

    // once again...
    REQUIRE(prog.call_function<segfaulting_fn>("segfaulting_fn") == UnexpectedReturn);

    // and a timeout as well for good measure...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);
}

TEST_CASE("Test that SYS_exit is properly thwarted") {
    asmgrader::Program prog(ASM_TESTS_EXEC, {});

    using enum asmgrader::ErrorKind;

    REQUIRE(prog.call_function<exiting_fn>("exiting_fn", 42) == UnexpectedReturn);

    REQUIRE(prog.get_subproc().is_alive());
}

TEST_CASE("Test that executables with 'weird' file names work") {
    asmgrader::Program prog(ASM_TESTS_WEIRD_NAME_EXEC, {});

    using enum asmgrader::ErrorKind;

    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // and again...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // ensure that calling other functions still works
    REQUIRE(prog.call_function<sum>("sum", 128, 42) == 170ull);

    // once again...
    REQUIRE(prog.call_function<segfaulting_fn>("segfaulting_fn") == UnexpectedReturn);

    REQUIRE(prog.call_function<exiting_fn>("exiting_fn", 42) == UnexpectedReturn);

    REQUIRE(prog.get_subproc().is_alive());
}
