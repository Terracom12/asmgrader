#pragma once

#include "api/assignment.hpp" // IWYU pragma: export
#include "api/test_base.hpp"  // IWYU pragma: export
#include "common/macros.hpp"
#include "grading_session.hpp" // IWYU pragma: export
#include "logging.hpp"
#include "registrars/auto_registrars.hpp" // IWYU pragma: export
#include "test_context.hpp"

// Some macros to substantially simplify test case development

#define ASSIGNMENT(cpp_identifier, name, executable)                                                                   \
    const static Assignment& cpp_identifier = /* NOLINT(misc-use-anonymous-namespace)*/                                \
        ::asmgrader::GlobalRegistrar::get().add(Assignment{name, executable});

#define TEST_IMPL(ident, name, /*metadata attributes*/...)                                                             \
    namespace {                                                                                                        \
    class ident final : public ::asmgrader::TestBase                                                                   \
    {                                                                                                                  \
    public:                                                                                                            \
        using TestBase::TestBase;                                                                                      \
        void run(::asmgrader::TestContext& ctx) override;                                                              \
    };                                                                                                                 \
    using namespace ::asmgrader::metadata; /*NOLINT(google-build-using-namespace)*/                                    \
    constexpr auto CONCAT(ident,                                                                                       \
                          __metadata) = ::asmgrader::metadata::create(::asmgrader::metadata::DEFAULT_METADATA,         \
                                                                      ::asmgrader::metadata::global_file_metadata()    \
                                                                          __VA_OPT__(, ) __VA_ARGS__);                 \
    const ::asmgrader::TestAutoRegistrar<ident> CONCAT(ident, __registrar){name, CONCAT(ident, __metadata)};           \
    }                                                                                                                  \
    void ident::run([[maybe_unused]] ::asmgrader::TestContext& ctx)

#define TEST(name, ...) TEST_IMPL(CONCAT(TEST__, __COUNTER__), name __VA_OPT__(, ) __VA_ARGS__)

// Simple way to ensure that no instructions are generated for prof-only tests when
// compiling for the students' version
#ifdef PROFESSOR_VERSION
#define PROF_ONLY_TEST(name, ...) TEST(name, ProfOnly __VA_OPT__(, ) __VA_ARGS__)
#else
#define PROF_ONLY_TEST(name, ...) [[maybe_unused]] static void CONCAT(prof_test_unused, __COUNTER__)(TestContext & ctx)
#endif // PROFESSOR_VERSION

#define REQUIRE(condition, ...)                                                                                        \
    __extension__({                                                                                                    \
        auto cond__var = condition;                                                                                    \
        bool cond__bool__val = static_cast<bool>(cond__var);                                                           \
        if (!cond__bool__val) {                                                                                        \
            LOG_DEBUG("Requirement failed: {}", cond__var);                                                            \
        }                                                                                                              \
        ctx.require(cond__bool__val, __VA_ARGS__ __VA_OPT__(, )::asmgrader::RequirementResult::DebugInfo{#condition}); \
    })

#define REQUIRE_EQ(lhs, rhs, ...)                                                                                      \
    __extension__({                                                                                                    \
        const auto& req_eq_lhs__ = (lhs);                                                                              \
        const auto& req_eq_rhs__ = (rhs);                                                                              \
        bool req_eq_res__ =                                                                                            \
            ctx.require(req_eq_lhs__ == req_eq_rhs__,                                                                  \
                        __VA_ARGS__ __VA_OPT__(, )::asmgrader::RequirementResult::DebugInfo{#lhs " == " #rhs});        \
        if (!req_eq_res__) {                                                                                           \
            LOG_DEBUG("{} != {}", (req_eq_lhs__), (req_eq_rhs__));                                                     \
        }                                                                                                              \
        req_eq_res__;                                                                                                  \
    })

#define FILE_METADATA(...)                                                                                             \
    namespace asmgrader::metadata {                                                                                    \
    consteval auto global_file_metadata() {                                                                            \
        using ::asmgrader::metadata::Assignment, metadata::Weight, metadata::ProfOnly;                                 \
        return ::asmgrader::metadata::Metadata{__VA_ARGS__};                                                           \
    }                                                                                                                  \
    } // namespace metadata
