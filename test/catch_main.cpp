#include "logging.hpp"

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    init_loggers();

    int result = Catch::Session().run(argc, argv);

    return result;
}
