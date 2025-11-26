#include "catch2_custom.hpp"

#include "api/tempfile.hpp"
#include "common/aliases.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

TEST_CASE("temp file creation and removal") {
    fs::path path = [] {
        asmgrader::TempFile tmp{};
        REQUIRE(fs::exists(tmp.path()));
        return tmp.path();
    }();

    REQUIRE_FALSE(fs::exists(path));

    auto paths = [] {
        asmgrader::TempFile tmp1{};
        REQUIRE(fs::exists(tmp1.path()));

        asmgrader::TempFile tmp2{};
        REQUIRE(fs::exists(tmp2.path()));

        asmgrader::TempFile tmp3{};
        REQUIRE(fs::exists(tmp3.path()));

        return std::vector<fs::path>{tmp1.path(), tmp2.path(), tmp3.path()};
    }();

    for (const auto& p : paths) {
        REQUIRE_FALSE(fs::exists(p));
    }

    path = asmgrader::TempFile{}.path();
    REQUIRE_FALSE(fs::exists(path));
}

TEST_CASE("temp files with specified permissions") {
    auto verify_perms = [](asmgrader::u16 perms) {
        asmgrader::TempFile tmp1{perms};
        REQUIRE(static_cast<asmgrader::u16>(fs::status(tmp1.path()).permissions()) == perms);
    };

    verify_perms(0000);
    verify_perms(0777);
    verify_perms(0700);
    verify_perms(0770);
    verify_perms(0666);
    verify_perms(0660);
    verify_perms(0606);
    verify_perms(0066);
    verify_perms(0006);
}

TEST_CASE("temp file reading and writing") {
    using Catch::Matchers::IsEmpty;

    asmgrader::TempFile tmp{};

    REQUIRE_THAT(tmp.read_all(), IsEmpty());
    tmp.truncate();
    REQUIRE_THAT(tmp.read_all(), IsEmpty());
    tmp.write("");
    REQUIRE_THAT(tmp.read_all(), IsEmpty());

    tmp.write("hello, world");
    REQUIRE(tmp.read_all() == "hello, world");

    tmp.write("\n123");
    REQUIRE(tmp.read_all() == "hello, world\n123");

    tmp.truncate();
    REQUIRE_THAT(tmp.read_all(), IsEmpty());
}
