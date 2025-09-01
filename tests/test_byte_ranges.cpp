#include "catch2_custom.hpp"

#include "common/aliases.hpp"
#include "common/bit_casts.hpp"
#include "common/byte.hpp"
#include "common/byte_vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/algorithm/fill_n.hpp>
#include <range/v3/algorithm/is_sorted.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <stdexcept>
#include <tuple>
#include <vector>

using asmgrader::NativeByteVector, asmgrader::Byte, asmgrader::to_bytes;
using namespace asmgrader::aliases;

using Catch::Matchers::RangeEquals, Catch::Matchers::IsEmpty;

TEST_CASE("NativeByteVector constructors") {
    const std::vector<Byte> source{1, 2, 3};

    REQUIRE_THAT(NativeByteVector(source.begin(), source.end()), RangeEquals(source));

    REQUIRE_THAT((NativeByteVector{1, 2, 3}), RangeEquals(source));

    REQUIRE_THAT(NativeByteVector(0, 1), IsEmpty());

    REQUIRE_THAT(NativeByteVector(1, 0x1), RangeEquals(std::vector<Byte>{1}));

    REQUIRE_THAT(NativeByteVector(3, 0xA), RangeEquals(std::vector<Byte>{0xA, 0xA, 0xA}));
}

TEST_CASE("NativeByteVector standard operations") {
    std::vector<Byte> vec{1, 2, 3};

    SECTION("Size access and resize") {
        REQUIRE(vec.size() == 3);
        vec.resize(10);
        REQUIRE(vec.size() == 10);
        vec.resize(0);
        REQUIRE(vec.size() == 0);
    }

    SECTION("Element access") {
        REQUIRE(vec[0] == Byte{1});
        REQUIRE(vec.at(1) == Byte{2});

        vec.at(0) = 0xAA;
        vec[1] = 0xF;

        REQUIRE(vec[0] == Byte{0xAA});
        REQUIRE(vec.at(1) == Byte{0xF});

        REQUIRE_THROWS_AS(vec.at(3), std::out_of_range);
        REQUIRE_THROWS_AS(vec.at(100), std::out_of_range);
    }

    SECTION("Adding elements") {
        // this fails as Byte(int) narrows and thus is not allowed in non-constexpr contexts
        // vec.emplace_back(0xA);

        vec.emplace_back(u8{0xA});
        vec.push_back(0xFF);
        REQUIRE_THAT(vec, RangeEquals(std::vector<Byte>{1, 2, 3, 0xA, 0xFF}));

        std::vector<Byte> other{0x12, 0x34};
        vec.insert(vec.cbegin() + 1, other.begin(), other.end());

        REQUIRE_THAT(vec, RangeEquals(std::vector<Byte>{1, 0x12, 0x34, 2, 3, 0xA, 0xFF}));
    }

    SECTION("Support for misc range-based operations") {
        ranges::fill_n(vec.begin(), 2, Byte{0xBB});
        REQUIRE_THAT(vec, RangeEquals(std::vector<Byte>{0xBB, 0xBB, 3}));

        const auto const_copy = vec;
        REQUIRE(ranges::count(const_copy, Byte{0xBB}) == 2);

        ranges::sort(vec);
        REQUIRE(ranges::is_sorted(vec));
    }
}

TEST_CASE("bit casting conversions") {
    struct S
    {
        int i;
        double d;
        constexpr bool operator==(const S&) const = default;
    };

    SECTION("from and to simple arg") {
        auto val = GENERATE(0x0, 0x1234, 0x0ABCDEF123456789);
        auto vec = to_bytes<NativeByteVector>(val);

        REQUIRE(vec.size() == sizeof(val));
        REQUIRE(vec.bit_cast_to<decltype(val)>() == val);
    }

    SECTION("from and to structured arg") {
        auto val = GENERATE(S{0, 0.0}, S{42, 3.141592}, S{999999, 0.000000001});
        auto vec = to_bytes<NativeByteVector>(val);

        REQUIRE(vec.size() == sizeof(val));
        REQUIRE(vec.bit_cast_to<decltype(val)>() == val);
    }

    SECTION("from and to simple multi args") {
        auto val = GENERATE(S{0, 0.0}, S{42, 3.141592}, S{999999, 0.000000001});
        auto vec = to_bytes<NativeByteVector>(val.i, val.d);

        REQUIRE(vec.size() == sizeof(int) + sizeof(double));
        REQUIRE(vec.bit_cast_to<int>() == val.i);
        REQUIRE(vec.bit_cast_to<int, double>() == std::tuple{val.i, val.d});
    }

    SECTION("from and to structured multi args") {
        struct G
        {
            S s;
            int i;
            double d;
        };

        auto val =
            GENERATE(G{{0, 0.0}, 0, 0.0}, G{{42, 3.141592}, 2837, 1.4141}, G{{999999, 0.000000001}, -12345, 354e300});
        auto vec = to_bytes<NativeByteVector>(val.s, val.i, val.d);

        REQUIRE(vec.size() == sizeof(S) + sizeof(int) + sizeof(double));
        REQUIRE(vec.bit_cast_to<S>() == val.s);
        REQUIRE(vec.bit_cast_to<S, int>() == std::tuple{val.s, val.i});
        REQUIRE(vec.bit_cast_to<S, int, double>() == std::tuple{val.s, val.i, val.d});
    }

    SECTION("from ranges") {
        std::array src{S{0, 0.0}, S{42, 3.141592}, S{999999, 0.000000001}};
        auto vec = to_bytes<NativeByteVector>(src);

        REQUIRE(vec.size() == sizeof(src));
        REQUIRE(vec.bit_cast_to<decltype(src)>() == src);

        std::vector<int> empty{};
        REQUIRE_THAT(to_bytes<NativeByteVector>(empty), IsEmpty());
    }
    SECTION("to ranges") {
        std::vector<u8> src{0x01, 0x23, 0x45, 0x67, 0x89};
        auto vec = to_bytes<NativeByteVector>(src);

        REQUIRE(vec.size() == src.size());
        REQUIRE_THAT(vec.to_range<std::vector<u8>>(), RangeEquals(src));
        REQUIRE_THAT(vec.to_range<std::vector<Byte>>(), RangeEquals(src));
        REQUIRE_THAT(vec.to_range<std::vector<i8>>(),
                     RangeEquals(src | ranges::views::transform([](u8 val) { return static_cast<i8>(val); })));
    }
}
