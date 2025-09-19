#include "logging.hpp"

#include <catch2/catch_session.hpp>
#include <libassert/assert.hpp>

#include <stdexcept>

void libassert_handler(const libassert::assertion_info& info) {
    LOG_ERROR(info.to_string());

    throw std::runtime_error(info.to_string());
}

int main(int argc, char* argv[]) {
    asmgrader::init_loggers();

    libassert::set_failure_handler(libassert_handler);

    int result = Catch::Session().run(argc, argv);

    return result;
}
