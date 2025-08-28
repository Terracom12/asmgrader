#pragma once

#include "output/sink.hpp"

namespace asmgrader {

class StdoutSink : public Sink
{
public:
    void write(std::string_view str) override;
    void flush() override;

    ~StdoutSink() override = default;
};

} // namespace asmgrader
