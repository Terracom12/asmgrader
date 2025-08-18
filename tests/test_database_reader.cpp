#include "catch2_custom.hpp"

#include "database_reader.hpp"

#include <range/v3/view/zip.hpp>

#include <array>
#include <filesystem>
#include <string_view>
#include <tuple>
#include <utility>

auto THIS_DIR = std::filesystem::path{__FILE__}.parent_path();

TEST_CASE("Read small database") {
    DatabaseReader reader{THIS_DIR / "small_database.csv"};

    auto res = reader.read();

    REQUIRE(res);

    constexpr std::array EXPECTED_NAMES = std::to_array<std::tuple<std::string_view, std::string_view>>({
        {"Matthew", "Reese"},
        {"John", "Doe"},
        {"Pope", "Francis"},
    });

    for (const auto& [expected_name, read_name] : ranges::views::zip(EXPECTED_NAMES, res->entries)) {
        REQUIRE(std::tie(read_name.first_name, read_name.last_name) == expected_name);
    }
}

TEST_CASE("Read bad_field database; ensure error") {
    DatabaseReader reader{THIS_DIR / "bad_field.csv"};

    REQUIRE_FALSE(reader.read());
}

TEST_CASE("Read non-existant database; ensure error") {
    DatabaseReader reader{THIS_DIR / "i-do-not-exist.csv"};

    REQUIRE_FALSE(reader.read());
}
