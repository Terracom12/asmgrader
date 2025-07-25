#pragma once

#include "registrars/auto_registrars.hpp" // IWYU pragma: export
#include "test/assignment.hpp"            // IWYU pragma: export
#include "test/test_base.hpp"             // IWYU pragma: export

// Some macros to substantially simplify test-case development

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define ASSIGNMENT(cpp_identifier, name, executable)                                                                   \
    const static Assignment& cpp_identifier = /* NOLINT(misc-use-anonymous-namespace)*/                                \
        GlobalRegistrar::get().add(Assignment{name, executable});

#define TEST_IMPL(ident, assignment, name, ...)                                                                        \
    namespace {                                                                                                        \
    class ident final : public TestBase                                                                                \
    {                                                                                                                  \
    public:                                                                                                            \
        using TestBase::TestBase;                                                                                      \
        void run(TestContext& ctx) override;                                                                           \
    };                                                                                                                 \
    const TestAutoRegistrar CONCAT(ident, __registrar){ident{assignment, name}, assignment};                           \
    }                                                                                                                  \
    void ident::run([[maybe_unused]] TestContext& ctx)

#define TEST(assignment, name, ...) TEST_IMPL(CONCAT(TEST__, __COUNTER__), assignment, name)

#define REQUIRE(...)
