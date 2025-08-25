#pragma once

#include "common/class_traits.hpp"
#include "grading_session.hpp"
#include "output/sink.hpp"
#include "user/program_options.hpp"

#include <string_view>

namespace asmgrader {

class Serializer : NonCopyable
{
public:
    explicit Serializer(Sink& sink, ProgramOptions::VerbosityLevel verbosity)
        : sink_{sink}
        , verbosity_{verbosity} {}

    virtual ~Serializer() = default;

    virtual void on_requirement_result(const RequirementResult& data) = 0;
    virtual void on_test_begin(std::string_view test_name) = 0;
    virtual void on_test_result(const TestResult& data) = 0;
    virtual void on_assignment_result(const AssignmentResult& data) = 0;
    virtual void on_metadata() = 0;

    virtual void finalize() = 0;

protected:
    // TODO: yes, this is bad practice. Will redesign later
    Sink& sink_;
    ProgramOptions::VerbosityLevel verbosity_;
};

} // namespace asmgrader
