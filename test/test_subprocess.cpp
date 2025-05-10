#include <catch2/catch_test_macros.hpp>
#include <string>

#include "logging.hpp"
#include "subprocess/subprocess.hpp"

TEST_CASE("Read /bin/echo stdout") {
    using namespace std::chrono_literals;
    Subprocess proc("/bin/echo", {"-n", "Hello", "world!"});

    std::string output = proc.read_stdout(50ms);

    REQUIRE(output == "Hello world!");
}

TEST_CASE("Interact with /bin/cat") {
    using namespace std::chrono_literals;

    Subprocess proc("/bin/cat", {});

    proc.send_stdin("Goodbye dog...");
    std::string output = proc.read_stdout(10ms);

    LOG_DEBUG("READ: {:?}", output);

    REQUIRE(output == "Goodbye dog...");
}
