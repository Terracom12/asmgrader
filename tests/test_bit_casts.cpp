#include "catch2_custom.hpp"

#include "common/aliases.hpp"
#include "common/bit_casts.hpp"
#include "common/byte_array.hpp"
#include "common/byte_vector.hpp"
#include "common/os.hpp"
#include "common/to_static_range.hpp"

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <fmt/format.h>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/starts_with.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view_adaptor.hpp>

#include <array>
#include <string_view>
#include <tuple>
#include <vector>

using asmgrader::as_bytes, asmgrader::to_bytes, asmgrader::ByteArray;
using enum asmgrader::EndiannessKind;

using namespace asmgrader::aliases;

using asmgrader::NativeByteVector, asmgrader::NativeByteArray, asmgrader::ByteArray, asmgrader::ByteVector;
using asmgrader::static_to;
using Catch::Matchers::RangeEquals, Catch::Matchers::StartsWith;

// data / expected_msb / expected_lsb
constexpr auto int_tests = make_tests<u32, NativeByteArray<4>, NativeByteArray<4>>( //
    {
        {
            // data
            0,
            // expected_msb
            {
                0x00, 0x00, 0x00, 0x00, // 0
            },
            // expected_lsb
            {
                0x00, 0x00, 0x00, 0x00, // 0
            } //
        },
        {
            // data
            1,
            // expected_msb
            {
                0x00, 0x00, 0x00, 0x01, // 1
            },
            // expected_lsb
            {
                0x01, 0x00, 0x00, 0x00, // 1
            } //
        },
        {
            // data
            0x12345678,
            // expected_msb
            {
                0x12, 0x34, 0x56, 0x78, // 0x12345678
            },
            // expected_lsb
            {
                0x78, 0x56, 0x34, 0x12, // 0x12345678
            } //
        },
        {
            // data
            0xFFFFFFFF,
            // expected_msb
            {
                0xFF, 0xFF, 0xFF, 0xFF, // 0xFFFFFFFF
            },
            // expected_lsb
            {
                0xFF, 0xFF, 0xFF, 0xFF, // 0xFFFFFFFF
            } //
        },
    });

// data / expected_msb / expected_lsb
constexpr auto int_tuple_tests = make_tests<std::tuple<u8, u16, u32>, NativeByteArray<7>, NativeByteArray<7>>( //
    {
        {
            // data
            {0, 0, 0},
            // expected_msb
            {
                0x00,                  // 0
                0x00, 0x00,            // 0
                0x00, 0x00, 0x00, 0x00 // 0
            },
            // expected_lsb
            {
                0x00,                  // 0
                0x00, 0x00,            // 0
                0x00, 0x00, 0x00, 0x00 // 0
            },
        },
        {
            // data
            {1, 2, 3},
            // expected_msb
            {
                0x01,                  // 1
                0x00, 0x02,            // 2
                0x00, 0x00, 0x00, 0x03 // 3
            },
            // expected_lsb
            {
                0x01,                  // 1
                0x02, 0x00,            // 2
                0x03, 0x00, 0x00, 0x00 // 3
            },
        },
        {
            // data
            {0x12, 0x3456, 0x789ABCDE},
            // expected_msb
            {
                0x12,                  // 0x12
                0x34, 0x56,            // 0x3456
                0x78, 0x9A, 0xBC, 0xDE // 0x789ABCDE
            },
            // expected_lsb
            {
                0x12,                  // 0x12
                0x56, 0x34,            // 0x3456
                0xDE, 0xBC, 0x9A, 0x78 // 0x789ABCDE
            },
        },
        {
            // data
            {0xFF, 0xFFFF, 0xFFFFFFFF},
            // expected_msb
            {
                0xFF,                  // 0xFF
                0xFF, 0xFF,            // 0xFFFF
                0xFF, 0xFF, 0xFF, 0xFF // 0xFFFFFFFF
            },
            // expected_lsb
            {
                0xFF,                  // 0xFF
                0xFF, 0xFF,            // 0xFFFF
                0xFF, 0xFF, 0xFF, 0xFF // 0xFFFFFFFF
            },
        },
    });

// data / expected_msb / expected_lsb
constexpr auto int_array_tests = make_tests<std::array<u32, 3>, NativeByteArray<12>, NativeByteArray<12>>( //
    {
        {
            // data
            {1, 2, 3},
            // expected_msb
            {
                0x00, 0x00, 0x00, 0x01, // 1
                0x00, 0x00, 0x00, 0x02, // 2
                0x00, 0x00, 0x00, 0x03  // 3
            },
            // expected_lsb
            {
                0x01, 0x00, 0x00, 0x00, // 1
                0x02, 0x00, 0x00, 0x00, // 2
                0x03, 0x00, 0x00, 0x00  // 3
            } //
        },
        {
            // data
            {0x1234, 0x10000000, 0xFFFFFFFF},
            // expected_msb
            {
                0x00, 0x00, 0x12, 0x34, // 0x1234
                0x10, 0x00, 0x00, 0x00, // 0x10000000
                0xFF, 0xFF, 0xFF, 0xFF  // 0xFFFFFFFF
            },
            // expected_lsb
            {
                0x34, 0x12, 0x00, 0x00, // 0x1234
                0x00, 0x00, 0x00, 0x10, // 0x10000000
                0xFF, 0xFF, 0xFF, 0xFF  // 0xFFFFFFFF
            } //
        },
    });

// data / expected
constexpr auto string_tests = make_tests<std::string_view, NativeByteArray<3>>( //
    {
        {
            // data
            "",
            // expected
            {},
        },
        {
            // data (capture '\0')
            {"", 1},
            // expected
            {0x00},
        },
        {
            // data
            "abc",
            // expected
            {'a', 'b', 'c'},
        },
    });

TEST_CASE("to_bytes") {
    STATIC_TABLE_TESTS( //
        int_tests, (data, expected_msb, expected_lsb), ({
            constexpr auto msb_bytes = to_bytes<ByteArray, Big>(data);
            constexpr auto lsb_bytes = to_bytes<ByteArray, Little>(data);

            STATIC_REQUIRE(ranges::equal(msb_bytes, expected_msb));
            STATIC_REQUIRE(ranges::equal(lsb_bytes, expected_lsb));
        }));

    STATIC_TABLE_TESTS( //
        int_tuple_tests, (data, expected_msb, expected_lsb), ({
            constexpr auto msb_bytes = std::apply([](auto... ints) { return to_bytes<ByteArray, Big>(ints...); }, data);
            constexpr auto lsb_bytes =
                std::apply([](auto... ints) { return to_bytes<ByteArray, Little>(ints...); }, data);

            STATIC_REQUIRE(ranges::equal(msb_bytes, expected_msb));
            STATIC_REQUIRE(ranges::equal(lsb_bytes, expected_lsb));
        }));

    STATIC_TABLE_TESTS( //
        int_array_tests, (data, expected_msb, expected_lsb), ({
            constexpr auto msb_bytes = to_bytes<ByteArray, Big>(data);
            constexpr auto lsb_bytes = to_bytes<ByteArray, Little>(data);

            STATIC_REQUIRE(ranges::equal(msb_bytes, expected_msb));
            STATIC_REQUIRE(ranges::equal(lsb_bytes, expected_lsb));
        }));

    STATIC_TABLE_TESTS( //
        string_tests, (data, expected_bytes), ({
            constexpr auto msb_bytes = to_bytes<ByteArray, data.size(), Big>(data);
            constexpr auto lsb_bytes = to_bytes<ByteArray, data.size(), Little>(data);

            // It's a string, so byte ordering should not be relevant
            STATIC_REQUIRE(ranges::starts_with(expected_bytes, msb_bytes));
            STATIC_REQUIRE(ranges::starts_with(expected_bytes, lsb_bytes));
        }));
}

TEST_CASE("as_bytes view") {
    // constexpr still not supported in many places in range-v3 D:

    SECTION("int_array_tests") {
        const auto& [data, expected_msb, expected_lsb] = GENERATE(from_range(int_array_tests));

        REQUIRE_THAT(data | as_bytes<Big>(), RangeEquals(expected_msb));
        REQUIRE_THAT(data | as_bytes<Little>(), RangeEquals(expected_lsb));
    }

    SECTION("string_tests") {
        const auto& [data, expected_bytes] = GENERATE(from_range(string_tests));

        // Agh.. why is Matchers::StartsWith only for strings??

        CHECKED_IF(data.empty()) {
            REQUIRE_THAT(expected_bytes, RangeEquals(decltype(expected_bytes){}));
        }
        CHECKED_ELSE(data.empty()) {
            REQUIRE(ranges::starts_with(expected_bytes, data | as_bytes<Big>()));
            REQUIRE(ranges::starts_with(expected_bytes, data | as_bytes<Little>()));
        }
    }
}
