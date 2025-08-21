#pragma once

#include "common/aliases.hpp"
#include "common/os.hpp"
#include "meta/integer.hpp"

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <gsl/util>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/transform.hpp>

#include <bit>
#include <concepts>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include <sys/user.h> // user_regs_struct, ([x86_64] user_fpregs_struct | [aarch64] user_fpsimd_struct)

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
};

} // namespace detail

/// General Purpose Register
struct GeneralRegister : detail::RegisterBaseImpl<std::uint64_t>
{
    /*implicit*/ constexpr operator u64() const { return value; } // NOLINT(google-explicit-constructor)
};

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

#define DEF_gen_register(z, num, _)                                                                                    \
    DEF_getter(x##num, regs[num]) DEF_getter(w##num, GeneralRegister{regs[num].as<u32>()})
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
    DEF_getter(q##num, vregs[num]) DEF_getter(d##num, FloatingPointRegister{vregs[num].as<u64>()})                     \
        DEF_getter(s##num, FloatingPointRegister{vregs[num].as<u32>()})                                                \
            DEF_getter(h##num, FloatingPointRegister{vregs[num].as<u16>()})
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
#endif
