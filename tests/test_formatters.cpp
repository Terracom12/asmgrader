#include "catch2_custom.hpp"

#include "common/formatters/enum.hpp" // IWYU pragma: keep
#include "common/formatters/macros.hpp"

#include <boost/describe/enum.hpp>
#include <fmt/base.h>

namespace {
enum class ECa {};
enum class ECb { A, B };
enum class ECc {};
enum class ECd { A, B };

enum Ea {};

enum Eb { A, B };

enum Ec {};

enum Ed { C, D };

struct Sa
{
};

struct Sb
{
    int a;
    double d;
};

struct Sc
{
    int a;
    Sb b;
};

} // namespace

FMT_SERIALIZE_ENUM(ECa);
FMT_SERIALIZE_ENUM(ECb, A, B);
FMT_SERIALIZE_ENUM(Ea);
FMT_SERIALIZE_ENUM(Eb, A, B);

FMT_SERIALIZE_CLASS(Sa);
FMT_SERIALIZE_CLASS(Sb, a, d);
FMT_SERIALIZE_CLASS(Sc, a, b);

TEST_CASE("fmt::formatter enum class specialization compile-time tests") {
    STATIC_REQUIRE(fmt::formattable<ECa>);
    STATIC_REQUIRE(fmt::formattable<ECb>);
    STATIC_REQUIRE(!fmt::formattable<ECc>);
    STATIC_REQUIRE(!fmt::formattable<ECd>);

    STATIC_REQUIRE(fmt::formattable<Ea>);
    STATIC_REQUIRE(fmt::formattable<Eb>);
    STATIC_REQUIRE(!fmt::formattable<Ec>);
    STATIC_REQUIRE(!fmt::formattable<Ed>);

    STATIC_REQUIRE(fmt::formattable<Sa>);
    STATIC_REQUIRE(fmt::formattable<Sb>);
    STATIC_REQUIRE(fmt::formattable<Sc>);
} // namespace ct_test
