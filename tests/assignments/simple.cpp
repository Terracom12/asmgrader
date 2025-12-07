#include <asmgrader/asmgrader.hpp>

using namespace asmgrader;

constexpr std::string_view exec_filename = [] {
    std::string_view str{ASM_TESTS_EXEC};
    auto idx = str.find_last_of('/');
    return str.substr(idx + 1);
}();

FILE_METADATA(Assignment("simple", exec_filename));

TEST("sum function") {
    AsmFunction sum = ctx.find_function<u64(u64, u64)>("sum");
    AsmFunction sum_and_write = ctx.find_function<void(u64, u64)>("sum_and_write");

    REQUIRE(sum(0, 0) == 0u, "0 + 0 = 0");
    REQUIRE(sum(1, 1) == 2u, "1 + 1 = 2");
    REQUIRE(sum(10000, 9999) == 19999u, "10000 + 9999 = 19999");

    // FIXME: Endianness?
    REQUIRE(sum_and_write(0xAB, 0x00), "function runs!");
    REQUIRE(ctx.get_stdout() == "\xAB\x00\x00\x00\x00\x00\x00\x00"sv, "stdout's cool");
    REQUIRE(sum_and_write(0x00, 0xCD), "function runs!");
    REQUIRE(ctx.get_stdout() == "\xCD\x00\x00\x00\x00\x00\x00\x00"sv, "stdout's cool");

    REQUIRE(sum_and_write(0xAB, 0xCD), "function runs!");
    // REQUIRE(ctx.get_stdout() == "\x01\0\0\0\0\0\0\0\0\xCD\0\0\0\0\0\0\0\0", "stdout's cool");

    REQUIRE(sum_and_write(0x0123456789ABCDEF, 0xFEDCBA9876543210), "function runs!");
    // REQUIRE(ctx.get_stdout() == "\x01\x23\x45\x67\x89\xAB\xCD\xEF\xFE\xDC\xBA\x98\x76\x54\x32\x10", "stdout's cool");

    auto syscalls = ctx.get_syscall_records();
    REQUIRE(syscalls.size() == 4u, "4 syscalls!");
}

TEST("exiting_fn") {
    AsmFunction exiting_fn = ctx.find_function<void(u64)>("exiting_fn");

    REQUIRE(exiting_fn(0) == ErrorKind::UnexpectedReturn, "We exited");
}

TEST("symbols") {
    AsmSymbol strHello = ctx.find_symbol<std::string>("strHello");
    AsmSymbol strGoodbye = ctx.find_symbol<std::string>("strGoodbye");

    REQUIRE(*strHello == "Hello, from assembly!\n", "symbol check");
    REQUIRE(*strGoodbye == "Goodbye, :(\n", "~cymbal~ symbol check!");
}
