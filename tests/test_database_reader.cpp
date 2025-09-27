#include "catch2_custom.hpp"

#include "database_reader.hpp"

#include <range/v3/view/zip.hpp>

#include <array>
#include <filesystem>
#include <string_view>
#include <tuple>

using path = std::filesystem::path;

TEST_CASE("Read small database") {
    asmgrader::DatabaseReader reader{path{RESOURCES_DIR} / "small_database.csv"};

    auto res = reader.read();

    REQUIRE(res);

    constexpr std::array EXPECTED_NAMES = std::to_array<std::tuple<std::string_view, std::string_view>>({
        {"Matthew", "Reese"},
        {"John", "Doe"},
        {"Pope", "Francis"},
        {"Jack Bryan", "O'Reily"},
        {"Carlos", "De La Cruz"},
        {"Billie-Rose", "Tao"},
    });

    for (const auto& [expected_name, read_name] : ranges::views::zip(EXPECTED_NAMES, *res)) {
        REQUIRE(std::tie(read_name.first_name, read_name.last_name) == expected_name);
    }
}

TEST_CASE("Read bad_field database; ensure error") {
    asmgrader::DatabaseReader reader{path{RESOURCES_DIR} / "bad_field.csv"};

    REQUIRE_FALSE(reader.read());
}

TEST_CASE("Read non-existant database; ensure error") {
    asmgrader::DatabaseReader reader{path{RESOURCES_DIR} / "i-do-not-exist.csv"};

    REQUIRE_FALSE(reader.read());
}

TEST_CASE("Read a database with CRLF line breaks") {
    // this database actually has MIXED line breaks: CRLF for some and just LF for others
    asmgrader::DatabaseReader reader{path{RESOURCES_DIR} / "crlf_database.csv"};

    auto res = reader.read();

    REQUIRE(res);

    constexpr std::array EXPECTED_NAMES = std::to_array<std::tuple<std::string_view, std::string_view>>({
        {"Matthew", "Reese"},
        {"John", "Doe"},
        {"Jane", "Doe"},
        {"Pope", "Francis"},
        {"Jack Bryan", "O'Reily"},
        {"Carlos", "De La Cruz"},
    });

    for (const auto& [expected_name, read_name] : ranges::views::zip(EXPECTED_NAMES, *res)) {
        REQUIRE(std::tie(read_name.first_name, read_name.last_name) == expected_name);
    }
}
