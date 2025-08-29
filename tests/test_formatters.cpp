#include "catch2_custom.hpp"

#include "common/formatters/enum.hpp" // IWYU pragma: keep

#include <boost/describe/enum.hpp>
#include <fmt/base.h>

namespace {
enum class SEa {};
enum class SEb { A, B };
enum class SEc {};
enum class SEd { A, B };

enum Ea {};

enum Eb { A, B };

enum Ec {};

enum Ed { C, D };

// NOLINTBEGIN
BOOST_DESCRIBE_ENUM(SEa);
BOOST_DESCRIBE_ENUM(SEb, A, B);
BOOST_DESCRIBE_ENUM(Ea);
BOOST_DESCRIBE_ENUM(Eb, A, B);
// NOLINTEND
} // namespace

TEST_CASE("fmt::formatter enum class specialization compile-time tests") {
    STATIC_REQUIRE(fmt::formattable<SEa>);
    STATIC_REQUIRE(fmt::formattable<SEb>);
    STATIC_REQUIRE(!fmt::formattable<SEc>);
    STATIC_REQUIRE(!fmt::formattable<SEd>);

    STATIC_REQUIRE(fmt::formattable<Ea>);
    STATIC_REQUIRE(fmt::formattable<Eb>);
    STATIC_REQUIRE(!fmt::formattable<Ec>);
    STATIC_REQUIRE(!fmt::formattable<Ed>);
} // namespace ct_test
