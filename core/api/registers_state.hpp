#pragma once

#include "boost/preprocessor/repetition/repeat_from_to.hpp"
#include "common/aliases.hpp"
#include "common/expected.hpp"
#include "common/os.hpp"
#include "common/static_string.hpp"
#include "meta/integer.hpp"

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/limits/repeat_256.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <gsl/util>
#include <range/v3/action/insert.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/set_algorithm.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <sys/user.h> // user_regs_struct, ([x86_64] user_fpregs_struct | [aarch64] user_fpsimd_struct)

/// All header only as this is relatively low level and we want operations to be fast (inlinable)

namespace detail {

template <typename BaseType>
struct RegisterBaseImpl
{
    BaseType value;

    template <std::integral IntType>
    constexpr IntType as() const {
        // FIXME: Something something endianness

        if (std::is_constant_evaluated()) {
            // Set all bits (either 2's complement, or undeflow of unsigned integer)
            constexpr auto ALL_BITS = static_cast<IntType>(-1);

            // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
            constexpr auto MASK = static_cast<std::uint64_t>(ALL_BITS);

            return static_cast<IntType>(MASK & value);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const IntType*>(&value);
    }

    constexpr auto operator<=>(const RegisterBaseImpl<BaseType>& rhs) const = default;
};

} // namespace detail

/// General Purpose Register
struct GeneralRegister : detail::RegisterBaseImpl<u64>
{
    // NOLINTNEXTLINE(google-explicit-constructor)
    /*implicit*/ constexpr operator u64() const { return value; }
};

static_assert(sizeof(GeneralRegister) == sizeof(u64));

struct FloatingPointRegister : detail::RegisterBaseImpl<u128>
{
    template </*std::floating_point*/ typename FPType>
    static constexpr FloatingPointRegister from(FPType val) {
        using IntType = meta::sized_uint_t<sizeof(FPType)>;
        auto int_val = std::bit_cast<IntType>(val);

        return {int_val};
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    /*implicit*/ constexpr operator double() const { return std::bit_cast<double>(static_cast<u64>(value)); }

    template </*std::floating_point*/ typename FPType>
    /*implicit*/ constexpr FPType as() const {
        using IntType = meta::sized_uint_t<sizeof(FPType)>;
        auto int_val = detail::RegisterBaseImpl<u128>::as<IntType>();

        return std::bit_cast<FPType>(int_val);
    }
};

static_assert(sizeof(FloatingPointRegister) == sizeof(u128));

struct RegistersState
{

    constexpr bool negative_set() const;
    constexpr bool zero_set() const;
    constexpr bool carry_set() const;
    constexpr bool overflow_set() const;

#if defined(ASMGRADER_AARCH64)
#define DEF_getter(name, expr)                                                                                         \
    constexpr auto name() const { return expr; }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NUM_GEN_REGISTERS 31
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NUM_FP_REGISTERS 32

    // General registers (x0-x30)
    std::array<GeneralRegister, NUM_GEN_REGISTERS> regs;

#define DEF_gen_register(z, num, _) DEF_getter(x##num, regs[num]) DEF_getter(w##num, static_cast<u32>(regs[num]))
    BOOST_PP_REPEAT(NUM_GEN_REGISTERS, DEF_gen_register, ~);
#undef DEF_gen_register

    DEF_getter(lr, x30()); // link register
    u64 sp;                // stack pointer
    u64 pc;                // program counter

    u64 pstate;

    // Specification of pstate (for nzcv) obtained from:
    //   https://developer.arm.com/documentation/ddi0601/2025-06/AArch64-Registers/NZCV--Condition-Flags
    static constexpr u64 NEGATIVE_FLAG_BIT = 0b1000;
    static constexpr u64 ZERO_FLAG_BIT = 0b0100;
    static constexpr u64 CARRY_FLAG_BIT = 0b0010;
    static constexpr u64 OVERFLOW_FLAG_BIT = 0b0001;

    constexpr u64 nzcv() const {
        constexpr u64 NZCV_BIT_OFF = 28;
        constexpr u64 NZCV_BIT_MASK = 0xF;

        return (pstate >> NZCV_BIT_OFF) & NZCV_BIT_MASK;
    }

    // Floating-point registers (Q0-Q31)
    std::array<FloatingPointRegister, NUM_FP_REGISTERS> vregs;

#define DEF_fp_register(z, num, _)                                                                                     \
    DEF_getter(q##num, vregs[num]) DEF_getter(d##num, vregs[num].as<u64>()) DEF_getter(s##num, vregs[num].as<u32>())   \
        DEF_getter(h##num, vregs[num].as<u16>())
    BOOST_PP_REPEAT(NUM_FP_REGISTERS, DEF_fp_register, ~);
#undef DEF_fp_register

    u32 fpsr; // Floating-point status register
    u32 fpcr; // Floating-point control register

#undef DEF_getter

    static constexpr RegistersState from(const user_regs_struct& regs, const user_fpsimd_struct& fpsimd_regs);
#elif defined(ASMGRADER_X86_64)

    // TODO: Different access modes for general registers (eax, ax, al, ah, etc.)
#define REGS_tuple_set                                                                                                 \
    BOOST_PP_TUPLE_TO_SEQ(15, (r15, r14, r13, r12, rbp, rbx, r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi))
#define DEF_gen_register(r, _, elem) GeneralRegister elem;
    BOOST_PP_SEQ_FOR_EACH(DEF_gen_register, _, REGS_tuple_set)
#undef DEF_gen_register

    u64 rip; // instruction pointer
    u64 rsp; // stack pointer

    u64 eflags; // flags register

    // Specification of eflags obtained from:
    //   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
    //   Volume 1 - 3.4.3.1 Status Flags
    static constexpr u64 CARRY_FLAG_BIT = 0b1 << 0;
    static constexpr u64 ZERO_FLAG_BIT = 0b1 << 6;
    static constexpr u64 SIGN_FLAG_BIT = 0b1 << 7;
    static constexpr u64 OVERFLOW_FLAG_BIT = 0b1 << 11;

    static constexpr RegistersState from(const user_regs_struct& regs, const user_fpregs_struct& fp_regs);
#endif
};

constexpr bool RegistersState::negative_set() const {
#if defined(ASMGRADER_AARCH64)
    return (nzcv() & NEGATIVE_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
    return (eflags & SIGN_FLAG_BIT) != 0U;
#endif
}

constexpr bool RegistersState::zero_set() const {
#if defined(ASMGRADER_AARCH64)
    return (nzcv() & ZERO_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
    return (eflags & ZERO_FLAG_BIT) != 0U;
#endif
}

constexpr bool RegistersState::carry_set() const {
#if defined(ASMGRADER_AARCH64)
    return (nzcv() & CARRY_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
    return (eflags & CARRY_FLAG_BIT) != 0U;
#endif
}

constexpr bool RegistersState::overflow_set() const {
#if defined(ASMGRADER_AARCH64)
    return (nzcv() & OVERFLOW_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
    return (eflags & OVERFLOW_FLAG_BIT) != 0U;
#endif
}

#if defined(ASMGRADER_AARCH64)
constexpr RegistersState RegistersState::from(const user_regs_struct& regs, const user_fpsimd_struct& fpsimd_regs) {
    RegistersState result{};

    ranges::copy(regs.regs | ranges::views::transform([](auto value) { return GeneralRegister{value}; }),
                 result.regs.begin());

    result.sp = regs.sp;
    result.pc = regs.pc;
    result.pstate = regs.pstate;

    ranges::copy(fpsimd_regs.vregs | ranges::views::transform([](auto value) { return FloatingPointRegister{value}; }),
                 result.vregs.begin());

    result.fpsr = fpsimd_regs.fpsr;
    result.fpcr = fpsimd_regs.fpcr;

    return result;
}

inline util::Expected<void, std::vector<std::string_view>> valid_pcs(const RegistersState& before_call,
                                                                     const RegistersState& after_call) noexcept {
    // PCS Docs (not official spec):
    //   https://developer.arm.com/documentation/102374/0102/Procedure-Call-Standard

    std::vector<std::string_view> err_changed_names;

    auto check_reg = [&](const auto& before, const auto& after, std::string_view name) {
        if (before != after) {
            err_changed_names.push_back(name);
        }
    };

#define CHECK_reg_nr(z, nr, pre) check_reg(before_call.pre##nr(), after_call.pre##nr(), #pre #nr);

    // General purpose regs X[19, 30) must be preserved
    BOOST_PP_REPEAT_FROM_TO(19, 30, CHECK_reg_nr, x)

    // Floating-point regs D[8, 32) must be preserved
    BOOST_PP_REPEAT_FROM_TO(8, 32, CHECK_reg_nr, d)

#undef CHECK_reg_nr

    // Stack pointer must be preserved
    // (caller-passed args are READ FROM, NOT POPPED from the stack by the callee)
    check_reg(before_call.sp(), after_call.sp(), "sp");

    if (err_changed_names.empty()) {
        return {};
    }

    return err_changed_names;
}

#elif defined(ASMGRADER_X86_64)

// A benefit of using the same names as user_regs_struct
#define COPY_gen_reg(r, base, elem) base.elem.value = regs.elem;

constexpr RegistersState RegistersState::from(const user_regs_struct& regs, const user_fpregs_struct& fp_regs) {
    RegistersState result{};

    BOOST_PP_SEQ_FOR_EACH(COPY_gen_reg, result, REGS_tuple_set);

    result.rip = regs.rip;
    result.rsp = regs.rsp;

    result.eflags = regs.eflags;

    // TODO: Support floating point on x86_64
    std::ignore = fp_regs;

    return result;
}

#undef COPY_gen_reg
#undef REGS_tuple_set

inline util::Expected<void, std::vector<std::string_view>> valid_pcs(const RegistersState& before_call,
                                                                     const RegistersState& after_call) noexcept {
    // Official (old, circa 2012) calling convention source:
    //   https://web.archive.org/web/20120913114312/http://www.x86-64.org/documentation/abi.pdf:

    std::vector<std::string_view> err_changed_names;

    auto check_reg = [&](const auto& before, const auto& after, std::string_view name) {
        if (before != after) {
            err_changed_names.push_back(name);
        }
    };

#define CHECK_reg_nr(z, nr, pre) check_reg(before_call.pre##nr, after_call.pre##nr, #pre #nr);

    // General purpose regs X[12, 36) must be preserved
    BOOST_PP_REPEAT_FROM_TO(12, 15, CHECK_reg_nr, r)

    // TODO: Check floating point and vectorized extension registers for x86_64 calling convention check

#undef CHECK_reg_nr

    // Stack pointer must be preserved
    // (caller-passed args are READ FROM, NOT POPPED from the stack by the callee)
    check_reg(before_call.rsp, after_call.rsp, "rsp");

    // rbx and rbp must be preserved
    check_reg(before_call.rbx, after_call.rbx, "rbx");
    check_reg(before_call.rbp, after_call.rbp, "rbp");

    if (err_changed_names.empty()) {
        return {};
    }

    return err_changed_names;
}
#endif
