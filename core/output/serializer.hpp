#pragma once

#include "grading_session.hpp"
#include "output/sink.hpp"
#include "common/class_traits.hpp"

class Serializer : util::NonCopyable
{
public:
    explicit Serializer(Sink& sink)
        : sink_{sink} {}

    virtual ~Serializer() = default;

    virtual void on_requirement_result(const RequirementResult& data) = 0;
    virtual void on_test_result(const TestResult& data) = 0;
    virtual void on_assignment_result(const AssignmentResult& data) = 0;
    virtual void on_metadata() = 0;

    virtual void finalize() = 0;

protected:
    // TODO: yes, this is bad practice. Will redesign later
    Sink& sink_;
};
