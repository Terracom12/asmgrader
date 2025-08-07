#pragma once

#include <string_view>

class Sink
{
public:
    virtual void write(std::string_view str) = 0;
    virtual void flush() = 0;

    virtual ~Sink() = default;
};
