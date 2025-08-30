#include "catch2_custom.hpp"

#include "api/metadata.hpp"

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <type_traits>

using namespace asmgrader::metadata;

// deduction guide
TEST_CASE("Metadata deduction guide") {
    STATIC_REQUIRE(std::same_as<decltype(Metadata{ProfOnly}), Metadata<ProfOnlyTag>>);
    STATIC_REQUIRE(std::same_as<decltype(Metadata{Assignment{"", ""}}), Metadata<Assignment>>);
}

TEST_CASE("Metadata copy and move ctors behave as expected") {
    // template ctor doesn't override copy and move ctors
    STATIC_REQUIRE(std::same_as<decltype(Metadata{Metadata{ProfOnly}}), Metadata<ProfOnlyTag>>);
    STATIC_REQUIRE(requires(Metadata<ProfOnlyTag> data) {
        {
            Metadata {
                data
            }
        } -> std::same_as<Metadata<ProfOnlyTag>>;
    });
}

namespace {

template <typename Lhs, typename Rhs>
struct Unchanged
{
    using DLhs = std::decay_t<Lhs>;
    using DRhs = std::decay_t<Rhs>;
    static constexpr bool value = std::same_as<DLhs, DRhs>;
};

template <typename Lhs, typename Rhs>
static constexpr bool unchanged_v = Unchanged<Lhs, Rhs>::value;

} // namespace

TEST_CASE("Missing attributes") {
    // missing attributes
    STATIC_REQUIRE(!Metadata{}.get<ProfOnlyTag>());
    STATIC_REQUIRE(!Metadata{Assignment{"", ""}}.get<ProfOnlyTag>());
    STATIC_REQUIRE(!Metadata{ProfOnly}.get<Assignment>());
    STATIC_REQUIRE(!Metadata{Assignment{"", ""}, ProfOnly}.get<Weight>());

    // missing + get_and
    STATIC_REQUIRE(!Metadata{Assignment{"", ""}, ProfOnly}.get_and<Weight>([](auto /*unused*/) {}));

    REQUIRE_FALSE(Metadata{}.get<ProfOnlyTag>());
    REQUIRE_FALSE(Metadata{Assignment{"", ""}}.get<ProfOnlyTag>());
    REQUIRE_FALSE(Metadata{ProfOnly}.get<Assignment>());
    REQUIRE_FALSE(Metadata{Assignment{"", ""}, ProfOnly}.get<Weight>());
}

TEST_CASE("Obtain present attributes") {
    REQUIRE(Metadata{ProfOnly}.get<ProfOnlyTag>() == ProfOnly);
    REQUIRE(Metadata{Assignment("", "")}.get<Assignment>() == Assignment("", ""));
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<ProfOnlyTag>() == ProfOnly);
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<Assignment>() == Assignment("", ""));
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<Assignment>() != Assignment("oh", "no"));
    REQUIRE(Metadata{Weight{5}, ProfOnly, Assignment("", "")}.get<Weight>() == Weight(5));
}

TEST_CASE("Attributes with get_and") {
    // missing + get_and
    REQUIRE_FALSE(Metadata{Assignment{"", ""}, ProfOnly}.get_and<Weight>([](auto /*unused*/) {}));

    REQUIRE(Metadata{ProfOnly}.get_and<ProfOnlyTag>([](ProfOnlyTag /*unused*/) { return 42; }) == 42);
    REQUIRE(Metadata{Assignment("", "")}.get<Assignment>() == Assignment("", ""));
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<ProfOnlyTag>() == ProfOnly);
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<Assignment>() == Assignment("", ""));
    REQUIRE(Metadata{ProfOnly, Assignment("", "")}.get<Assignment>() != Assignment("oh", "no"));
    REQUIRE(Metadata{Weight{5}, ProfOnly, Assignment("", "")}.get<Weight>() == Weight(5));
}

TEST_CASE("Combining attributes") {
    constexpr Metadata start{ProfOnly};

    STATIC_REQUIRE(unchanged_v<decltype(start), decltype(start | ProfOnly | ProfOnly)>);
    REQUIRE((start | ProfOnly).get<ProfOnlyTag>());

    REQUIRE((start | Weight{1}).get<Weight>() == Weight{1});
    REQUIRE((start | Weight{1} | Weight{2}).get<Weight>() == Weight{2});

    REQUIRE((start | Assignment{"no", "way"}).get<Assignment>() == Assignment{"no", "way"});
    REQUIRE((start | Assignment{"", ""} | Assignment{"no", "way"}).get<Assignment>() == Assignment{"no", "way"});
}

TEST_CASE("Combining metadata objects") {
    constexpr Metadata start{};
    constexpr Metadata prof_only_only{ProfOnly};
    constexpr Metadata weight_only{Weight{1}};
    constexpr Metadata assignment_only{Assignment{"", ""}};

    STATIC_REQUIRE(unchanged_v<decltype(start), decltype(start | start)>);
    STATIC_REQUIRE(unchanged_v<decltype(weight_only), decltype(weight_only | weight_only)>);

    REQUIRE_FALSE((start | start).get<ProfOnlyTag>());
    REQUIRE((start | prof_only_only).get<ProfOnlyTag>());

    REQUIRE((start | weight_only).get<Weight>() == Weight{1});
    REQUIRE((start | weight_only | Weight{2}).get<Weight>() == Weight{2});

    REQUIRE((start | assignment_only).get<Assignment>() == Assignment{"", ""});
    REQUIRE((start | assignment_only | Assignment{"no", "way"}).get<Assignment>() == Assignment{"no", "way"});

    [[maybe_unused]] constexpr Metadata constructed{start, prof_only_only, weight_only};
}
