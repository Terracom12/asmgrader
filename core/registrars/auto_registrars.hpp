#pragma once

#include "registrars/global_registrar.hpp"
#include "test/assignment.hpp"
#include "test/test_base.hpp"

#include <concepts>
#include <memory>

/// Helper class that, whem constructed, automatically registers an assignment to the global registrar
class AssignmentAutoRegistrar
{
public:
    explicit AssignmentAutoRegistrar(Assignment assignment) noexcept {
        GlobalRegistrar::get().add(std::move(assignment));
    }
};

/// Helper class that, whem constructed, automatically registers a test to the specified assignment
class TestAutoRegistrar
{
public:
    template <typename TestClass>
        requires(std::derived_from<TestClass, TestBase>)
    explicit TestAutoRegistrar(TestClass&& test, const Assignment& assignment) noexcept {
        assignment.add_test(std::make_unique<TestClass>(std::forward<TestClass>(test)));
    }
};
