#include "output/stdout_sink.hpp"

#include <iostream>
#include <string_view>

void StdoutSink::write(std::string_view str) {
    std::cout << str;
}

void StdoutSink::flush() {
    std::cout.flush();
}
