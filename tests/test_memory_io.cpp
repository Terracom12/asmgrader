#include "catch2_custom.hpp"

#include "logging.hpp"
#include "subprocess/memory/memory_io_base.hpp"
#include "subprocess/memory/memory_io_serde.hpp"
#include "util/byte_vector.hpp"
#include "util/timespec_operator_eq.hpp" // operator==(timespec, timespec)

#include <range/v3/algorithm.hpp>

#include <array>
#include <cstddef>
#include <ctime>
#include <memory>
#include <tuple>

template <std::size_t N>
class StubbedMemoryIO final : public MemoryIOBase
{
public:
    StubbedMemoryIO()
        : MemoryIOBase(-1) {}

private:
    util::Result<ByteVector> read_block_impl(std::uintptr_t address, std::size_t length) override {
        ASSERT(address + length < N);
        return ByteVector{begin(data_) + address, begin(data_) + address + length};
    }

    util::Result<void> write_block_impl(std::uintptr_t address, const ByteVector& data) override {
        ASSERT(address + data.size() < N);
        ranges::copy(data, begin(data_) + address);

        return {};
    }
    std::array<std::byte, N> data_{};
};

constexpr std::size_t MEM_SIZE = 1'024;
auto make_memory = []() -> std::unique_ptr<MemoryIOBase> { return std::make_unique<StubbedMemoryIO<MEM_SIZE>>(); };

TEST_CASE("Read and write arithmetic types and pointers") {
    auto mio = make_memory();

    SECTION("Signed integers") {
        std::int8_t a = 123;
        std::int16_t b = 0xABC;
        std::int32_t c = -32;
        std::int64_t d = -999999;

        std::ignore = mio->write(0, a);
        std::ignore = mio->write(1, b);
        std::ignore = mio->write(2, b);
        std::ignore = mio->write(4, c);
        std::ignore = mio->write(8, d);

        REQUIRE(mio->read<std::int8_t>(0) == a);
        REQUIRE(mio->read<std::int16_t>(2) == b);
        REQUIRE(mio->read<std::int32_t>(4) == c);
        REQUIRE(mio->read<std::int64_t>(8) == d);
    }

    SECTION("Unsigned integers") {
        std::uint64_t f = 0x0123456789ABCDEF;
        std::uint8_t g = 0xFF;

        std::ignore = mio->write(3, f);
        std::ignore = mio->write(2, g);

        REQUIRE(mio->read<std::uint64_t>(3) == f);
        REQUIRE(mio->read<std::uint8_t>(2) == g);
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
        MemoryIOBase* l = mio.get();

        std::ignore = mio->write(100, j);
        std::ignore = mio->write(108, k);
        std::ignore = mio->write(116, l);

        REQUIRE(mio->read<int*>(100) == j);
        REQUIRE(mio->read<int**>(108) == k);
        REQUIRE(mio->read<MemoryIOBase*>(116) == l);
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

    NonTermString<7> nnts = {"Goodbye"};
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
