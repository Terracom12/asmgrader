#include "catch2_custom.hpp"

#include "common/aliases.hpp"
#include "common/byte_vector.hpp"
#include "common/error_types.hpp"
#include "common/timespec_operator_eq.hpp" // operator==(timespec, timespec)
#include "logging.hpp"
#include "subprocess/memory/memory_io_base.hpp"
#include "subprocess/memory/memory_io_serde.hpp"
#include "subprocess/memory/non_terminated_str.hpp"

#include <libassert/assert.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/algorithm/copy.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <tuple>

using namespace asmgrader::aliases;

template <std::size_t N>
class StubbedMemoryIO final : public asmgrader::MemoryIOBase
{
public:
    StubbedMemoryIO()
        : MemoryIOBase(-1) {}

private:
    asmgrader::Result<asmgrader::ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override {
        ASSERT(address + length < N);
        return asmgrader::ByteVector{begin(data_) + address, begin(data_) + address + length};
    }

    asmgrader::Result<void> write_block_impl(std::uintptr_t address, const asmgrader::ByteVector& data) override {
        ASSERT(address + data.size() < N);
        ranges::copy(data, begin(data_) + address);

        return {};
    }

    std::array<std::byte, N> data_{};
};

constexpr std::size_t mem_size = 1'024;
auto make_memory = []() -> std::unique_ptr<asmgrader::MemoryIOBase> {
    return std::make_unique<StubbedMemoryIO<mem_size>>();
};

TEST_CASE("Read and write arithmetic types and pointers") {
    auto mio = make_memory();

    SECTION("Signed integers") {
        i8 a = 123;
        i16 b = 0xABC;
        i32 c = -32;
        i64 d = -999999;

        std::ignore = mio->write(0, a);
        std::ignore = mio->write(1, b);
        std::ignore = mio->write(2, b);
        std::ignore = mio->write(4, c);
        std::ignore = mio->write(8, d);

        REQUIRE(mio->read<i8>(0) == a);
        REQUIRE(mio->read<i16>(2) == b);
        REQUIRE(mio->read<i32>(4) == c);
        REQUIRE(mio->read<i64>(8) == d);
    }

    SECTION("Unsigned integers") {
        u64 f = 0x0123456789ABCDEF;
        u8 g = 0xFF;

        std::ignore = mio->write(3, f);
        std::ignore = mio->write(2, g);

        REQUIRE(mio->read<u64>(3) == f);
        REQUIRE(mio->read<u8>(2) == g);
    }

    SECTION("Floating point numbers") {
        float h = 3.14159f;
        double i = 1.414;

        std::ignore = mio->write(27, h);
        std::ignore = mio->write(128, i);

        REQUIRE(mio->read<float>(27) == h);
        REQUIRE(mio->read<double>(128) == i);
    }

    SECTION("Pointers") {
        int* j = nullptr;
        int** k = &j;
        asmgrader::MemoryIOBase* l = mio.get();

        std::ignore = mio->write(100, j);
        std::ignore = mio->write(108, k);
        std::ignore = mio->write(116, l);

        REQUIRE(mio->read<int*>(100) == j);
        REQUIRE(mio->read<int**>(108) == k);
        REQUIRE(mio->read<asmgrader::MemoryIOBase*>(116) == l);
    }
}

TEST_CASE("Read and write timespecs") {
    auto mio = make_memory();

    std::timespec tms[] = {{
                               .tv_sec = 0,
                               .tv_nsec = 0,
                           },
                           {
                               .tv_sec = 9999,
                               .tv_nsec = 0,
                           },
                           {
                               .tv_sec = 0,
                               .tv_nsec = 999999,
                           },
                           {
                               .tv_sec = 42,
                               .tv_nsec = 4096,
                           }};

    for (std::uintptr_t addr = 0; const auto& tm : tms) {
        std::ignore = mio->write(addr, tm);

        addr += sizeof(std::timespec);
    }

    for (std::uintptr_t addr = 0; const auto& tm : tms) {
        REQUIRE(mio->read<std::timespec>(addr) == tm);

        addr += sizeof(std::timespec);
    }
}

TEST_CASE("Read and write strings") {
    auto mio = make_memory();

    std::string s1 = "Hello, world!";
    std::string s2 = "";

    std::ignore = mio->write(0, s1);
    std::ignore = mio->write(s1.size() + 1, s2);

    REQUIRE(mio->read<char>(s1.size()) == '\0');
    REQUIRE(mio->read<char>(s1.size() + 1 + s2.size()) == '\0');

    asmgrader::NonTermString<7> nnts = {"Goodbye"};
    std::ignore = mio->write(0, nnts);
    REQUIRE(mio->read<char>(nnts.LENGTH) != '\0');
}

TEST_CASE("Dereference pointers") {
    auto mio = make_memory();

    int i = 123;
    int* i_ptr = reinterpret_cast<int*>(83);
    int** i_ptr_ptr = reinterpret_cast<int**>(976);

    std::ignore = mio->write(1000, i_ptr_ptr);
    std::ignore = mio->write(reinterpret_cast<std::uintptr_t>(i_ptr_ptr), i_ptr);
    std::ignore = mio->write(reinterpret_cast<std::uintptr_t>(i_ptr), i);

    REQUIRE(mio->read_deref_all<int**>(1000) == i);
}
