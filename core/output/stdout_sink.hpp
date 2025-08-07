#pragma once

#include "output/sink.hpp"

class StdoutSink : public Sink
{
public:
    void write(std::string_view str) override;
    void flush() override;

    ~StdoutSink() override = default;
};
