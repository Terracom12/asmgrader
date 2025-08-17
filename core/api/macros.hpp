#pragma once

#include "api/assignment.hpp"  // IWYU pragma: export
#include "api/test_base.hpp"   // IWYU pragma: export
#include "grading_session.hpp" // IWYU pragma: export
#include "logging.hpp"
#include "registrars/auto_registrars.hpp" // IWYU pragma: export
#include "util/macros.hpp"

// Some macros to substantially simplify test case development

#define ASSIGNMENT(cpp_identifier, name, executable)                                                                   \
    const static Assignment& cpp_identifier = /* NOLINT(misc-use-anonymous-namespace)*/                                \
        GlobalRegistrar::get().add(Assignment{name, executable});

#define TEST_IMPL(ident, name, /*metadata attributes*/...)                                                             \
    namespace {                                                                                                        \
    class ident final : public TestBase                                                                                \
    {                                                                                                                  \
    public:                                                                                                            \
        using TestBase::TestBase;                                                                                      \
        void run(TestContext& ctx) override;                                                                           \
    };                                                                                                                 \
    constexpr auto CONCAT(ident, __metadata) = metadata::create(metadata::DEFAULT_METADATA,                            \
                                                                metadata::global_file_metadata() __VA_OPT__(, )        \
                                                                    __VA_ARGS__);                                      \
    const TestAutoRegistrar<ident> CONCAT(ident, __registrar){name, CONCAT(ident, __metadata)};                        \
    }                                                                                                                  \
    void ident::run([[maybe_unused]] TestContext& ctx)

#define TEST(name, ...) TEST_IMPL(CONCAT(TEST__, __COUNTER__), name)

#define REQUIRE(condition, ...)                                                                                        \
    __extension__({                                                                                                    \
        auto cond__var = condition;                                                                                    \
        bool cond__bool__val = static_cast<bool>(cond__var);                                                           \
        if (!cond__bool__val) {                                                                                        \
            LOG_DEBUG("Requirement failed: {}", cond__var);                                                            \
        }                                                                                                              \
        ctx.require(cond__bool__val, __VA_ARGS__ __VA_OPT__(, ) RequirementResult::DebugInfo{#condition});             \
    })

#define REQUIRE_EQ(lhs, rhs, ...)                                                                                      \
    __extension__({                                                                                                    \
        const auto& req_eq_lhs__ = (lhs);                                                                              \
        const auto& req_eq_rhs__ = (rhs);                                                                              \
        bool req_eq_res__ = ctx.require(req_eq_lhs__ == req_eq_rhs__,                                                  \
                                        __VA_ARGS__ __VA_OPT__(, ) RequirementResult::DebugInfo{#lhs " == " #rhs});    \
        if (!req_eq_res__) {                                                                                           \
            LOG_DEBUG("{} != {}", (req_eq_lhs__), (req_eq_rhs__));                                                     \
        }                                                                                                              \
        req_eq_res__;                                                                                                  \
    })

#define FILE_METADATA(...)                                                                                             \
    namespace metadata {                                                                                               \
    consteval auto global_file_metadata() {                                                                            \
        using metadata::Assignment, metadata::Weight, metadata::ProfOnly;                                              \
        return Metadata{__VA_ARGS__};                                                                                  \
    }                                                                                                                  \
    } // namespace metadata
