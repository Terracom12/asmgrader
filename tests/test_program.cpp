#include "catch2_custom.hpp"

#include "program/program.hpp"
#include "util/error_types.hpp"

#include <cstdint>

using sum = std::uint64_t(std::uint64_t, std::uint64_t);
using sum_and_write = void(std::uint64_t, std::uint64_t);
using timeout_fn = void();
using segfaulting_fn = void();
using exiting_fn = void(std::uint64_t);

TEST_CASE("Try to call functions that don't exist") {
    Program prog(ASM_TESTS_EXEC, {});

    using util::ErrorKind::UnresolvedSymbol;

    REQUIRE(prog.call_function<void(void)>("") == UnresolvedSymbol);

    REQUIRE(prog.call_function<void(void)>("abc123") == UnresolvedSymbol);

    REQUIRE(prog.call_function<void(void)>("_sum") == UnresolvedSymbol);
}

TEST_CASE("Call sum function") {
    Program prog(ASM_TESTS_EXEC, {});
    util::Result<std::uint64_t> res;

    res = prog.call_function<sum>("sum", 0, 0);
    REQUIRE(res == 0ull);

    res = prog.call_function<sum>("sum", 1, 2);
    REQUIRE(res == 3ull);

    // unsigned, but overflow with 2's complement gives values as we would expect
    res = prog.call_function<sum>("sum", -1, -12);
    REQUIRE(res == static_cast<std::uint64_t>(-13));
}

TEST_CASE("Call sum_and_write function") {
    Program prog(ASM_TESTS_EXEC, {});

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
    Program prog(ASM_TESTS_EXEC, {});

    using util::ErrorKind::TimedOut;

    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // and again...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);

    // ensure that calling other functions still works
    REQUIRE(prog.call_function<sum>("sum", 128, 42) == 170ull);

    // and one mre time for good measure...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);
}

TEST_CASE("Test that segfaults are essentially ignored") {
    Program prog(ASM_TESTS_EXEC, {});

    using enum util::ErrorKind;

    REQUIRE(prog.call_function<segfaulting_fn>("segfaulting_fn") == UnexpectedReturn);

    // ensure that calling other functions still works
    REQUIRE(prog.call_function<sum>("sum", 128, 42) == 170ull);

    // once again...
    REQUIRE(prog.call_function<segfaulting_fn>("segfaulting_fn") == UnexpectedReturn);

    // and a timeout as well for good measure...
    REQUIRE(prog.call_function<timeout_fn>("timeout_fn") == TimedOut);
}

TEST_CASE("Test that SYS_exit is properly thwarted") {
    Program prog(ASM_TESTS_EXEC, {});

    using enum util::ErrorKind;

    REQUIRE(prog.call_function<exiting_fn>("exiting_fn", 42) == UnexpectedReturn);

    REQUIRE(prog.get_subproc().is_alive());
}
