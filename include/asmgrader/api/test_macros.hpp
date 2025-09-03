#pragma once

// Macro names in this file are short and may conflict with identifiers in 3rd-party libraries
// known conflicts with range-v3, so we just make sure to include it before
#include <range/v3/all.hpp> // IWYU pragma: export
//

#include <asmgrader/api/assignment.hpp>  // IWYU pragma: export
#include <asmgrader/api/requirement.hpp> // IWYU pragma: export
#include <asmgrader/api/test_base.hpp>   // IWYU pragma: export
#include <asmgrader/api/test_context.hpp>
#include <asmgrader/common/macros.hpp>
#include <asmgrader/grading_session.hpp> // IWYU pragma: export
#include <asmgrader/logging.hpp>
#include <asmgrader/registrars/auto_registrars.hpp> // IWYU pragma: export

#include <boost/preprocessor/repetition/repeat.hpp>

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

/// Define a test that runs only in professor mode.
///
/// Usage of this is prefered over `TEST("...", ProfOnly)` because with this macro, the test body and name will
/// not show up in the final binary. This might allow some determined students to figure out what tests are run
/// on the professor's end through some in-depth analysis.
#ifdef PROFESSOR_VERSION
#define PROF_ONLY_TEST(name, ...) TEST(name, ProfOnly __VA_OPT__(, ) __VA_ARGS__)
#else
#define PROF_ONLY_TEST(name, ...) [[maybe_unused]] static void CONCAT(prof_test_unused, __COUNTER__)(TestContext & ctx)
#endif // PROFESSOR_VERSION

// NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
#define REQUIRE_IMPL(condition, unq_ident, ...)                                                                        \
    do { /* NOLINT(cppcoreguidelines-avoid-do-while) */                                                                \
        const auto& CONCAT(op_, unq_ident) = ::asmgrader::exprs::make<::asmgrader::exprs::Noop>((condition));          \
        const auto& unq_ident =                                                                                        \
            ::asmgrader::Requirement{CONCAT(op_, unq_ident) __VA_OPT__(, ONLY_FIRST(__VA_ARGS__))};                    \
        bool CONCAT(bool_, unq_ident) = ctx.require(unq_ident, ::asmgrader::RequirementResult::DebugInfo{#condition}); \
    } while (false);

#define REQUIRE(condition, ...) REQUIRE_IMPL(condition, CONCAT(require_unq_, __COUNTER__), __VA_ARGS__)

// FIXME: DRY

#define REQUIRE_EQ_IMPL(lhs, rhs, unq_ident, ...)                                                                      \
    do { /* NOLINT(cppcoreguidelines-avoid-do-while) */                                                                \
        const auto& CONCAT(op_, unq_ident) = ::asmgrader::exprs::make<::asmgrader::exprs::Equal>((lhs), (rhs));        \
        const auto& unq_ident =                                                                                        \
            ::asmgrader::Requirement{CONCAT(op_, unq_ident) __VA_OPT__(, ONLY_FIRST(__VA_ARGS__))};                    \
        bool CONCAT(bool_, unq_ident) =                                                                                \
            ctx.require(unq_ident, ::asmgrader::RequirementResult::DebugInfo{#lhs " == " #rhs});                       \
    } while (false);

#define REQUIRE_EQ(lhs, rhs, ...) REQUIRE_EQ_IMPL(lhs, rhs, CONCAT(require_eq_unq_, __COUNTER__), __VA_ARGS__)

#define FILE_METADATA(...)                                                                                             \
    namespace asmgrader::metadata {                                                                                    \
    consteval auto global_file_metadata() {                                                                            \
        using namespace ::asmgrader; /* NOLINT(google-build-using-namespace) */                                        \
        return ::asmgrader::metadata::Metadata{__VA_ARGS__};                                                           \
    }                                                                                                                  \
    } // namespace metadata

// Macros for registers and flags

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REGS_MACRO_IMPL(which) ctx.get_registers().which

#define W0 REGS_MACRO_IMPL(w0)
#define X0 REGS_MACRO_IMPL(x0)
#define W1 REGS_MACRO_IMPL(w1)
#define X1 REGS_MACRO_IMPL(x1)
#define W2 REGS_MACRO_IMPL(w2)
#define X2 REGS_MACRO_IMPL(x2)
#define W3 REGS_MACRO_IMPL(w3)
#define X3 REGS_MACRO_IMPL(x3)
#define W4 REGS_MACRO_IMPL(w4)
#define X4 REGS_MACRO_IMPL(x4)
#define W5 REGS_MACRO_IMPL(w5)
#define X5 REGS_MACRO_IMPL(x5)
#define W6 REGS_MACRO_IMPL(w6)
#define X6 REGS_MACRO_IMPL(x6)
#define W7 REGS_MACRO_IMPL(w7)
#define X7 REGS_MACRO_IMPL(x7)
#define W8 REGS_MACRO_IMPL(w8)
#define X8 REGS_MACRO_IMPL(x8)
#define W9 REGS_MACRO_IMPL(w9)
#define X9 REGS_MACRO_IMPL(x9)
#define W10 REGS_MACRO_IMPL(w10)
#define X10 REGS_MACRO_IMPL(x10)
#define W11 REGS_MACRO_IMPL(w11)
#define X11 REGS_MACRO_IMPL(x11)
#define W12 REGS_MACRO_IMPL(w12)
#define X12 REGS_MACRO_IMPL(x12)
#define W13 REGS_MACRO_IMPL(w13)
#define X13 REGS_MACRO_IMPL(x13)
#define W14 REGS_MACRO_IMPL(w14)
#define X14 REGS_MACRO_IMPL(x14)
#define W15 REGS_MACRO_IMPL(w15)
#define X15 REGS_MACRO_IMPL(x15)
#define W16 REGS_MACRO_IMPL(w16)
#define X16 REGS_MACRO_IMPL(x16)
#define W17 REGS_MACRO_IMPL(w17)
#define X17 REGS_MACRO_IMPL(x17)
#define W18 REGS_MACRO_IMPL(w18)
#define X18 REGS_MACRO_IMPL(x18)
#define W19 REGS_MACRO_IMPL(w19)
#define X19 REGS_MACRO_IMPL(x19)
#define W20 REGS_MACRO_IMPL(w20)
#define X20 REGS_MACRO_IMPL(x20)
#define W21 REGS_MACRO_IMPL(w21)
#define X21 REGS_MACRO_IMPL(x21)
#define W22 REGS_MACRO_IMPL(w22)
#define X22 REGS_MACRO_IMPL(x22)
#define W23 REGS_MACRO_IMPL(w23)
#define X23 REGS_MACRO_IMPL(x23)
#define W24 REGS_MACRO_IMPL(w24)
#define X24 REGS_MACRO_IMPL(x24)
#define W25 REGS_MACRO_IMPL(w25)
#define X25 REGS_MACRO_IMPL(x25)
#define W26 REGS_MACRO_IMPL(w26)
#define X26 REGS_MACRO_IMPL(x26)
#define W27 REGS_MACRO_IMPL(w27)
#define X27 REGS_MACRO_IMPL(x27)
#define W28 REGS_MACRO_IMPL(w28)
#define X28 REGS_MACRO_IMPL(x28)
#define W29 REGS_MACRO_IMPL(w29)
#define X29 REGS_MACRO_IMPL(x29)
#define W30 REGS_MACRO_IMPL(w30)
#define X30 REGS_MACRO_IMPL(x30)

#define FP REGS_MACRO_IMPL(fp)
#define LR REGS_MACRO_IMPL(lr)
#define SP REGS_MACRO_IMPL(sp)
#define PC REGS_MACRO_IMPL(pc)

#define PSTATE REGS_MACRO_IMPL(pstate)
#define N PSTATE.n()
#define Z PSTATE.z()
#define C PSTATE.c()
#define V PSTATE.v()

// TODO: x86-64 accessors
