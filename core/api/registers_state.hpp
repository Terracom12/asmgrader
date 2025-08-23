#pragma once

#include "boost/preprocessor/repetition/repeat_from_to.hpp"
#include "common/aliases.hpp"
#include "common/expected.hpp"
#include "common/formatters/debug.hpp"
#include "common/os.hpp"
#include "meta/integer.hpp"

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gsl/util>
#include <gsl/zstring>
#include <range/v3/action/insert.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/utility/memory.hpp>
#include <range/v3/view/set_algorithm.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
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

template <>
struct fmt::formatter<GeneralRegister> : DebugFormatter
{
    auto format(const GeneralRegister& from, format_context& ctx) const {
        if (is_debug_format) {
            return format_to(ctx.out(), "{0:>{1}} | 0x{0:016X} | 0b{0:064B}", from.value,
                             std::numeric_limits<decltype(from.value)>::digits10);
        }

        return format_to(ctx.out(), "{}", from.value);
    }
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

    using detail::RegisterBaseImpl<u128>::as;

    template <std::floating_point FPType>
    constexpr FPType as() const {
        using IntType = meta::sized_uint_t<sizeof(FPType)>;
        auto int_val = detail::RegisterBaseImpl<u128>::as<IntType>();

        return std::bit_cast<FPType>(int_val);
    }
};

static_assert(sizeof(FloatingPointRegister) == sizeof(u128));

template <>
struct fmt::formatter<FloatingPointRegister> : DebugFormatter
{
    auto format(const FloatingPointRegister& from, format_context& ctx) const {
        if (is_debug_format) {
            return format_to(ctx.out(), "{}", double{from});
        }

        const auto max_sz = formatted_size("{}", std::numeric_limits<double>::min());
        return format_to(ctx.out(), "{:>{}} (f64) | 0x{:032X}", double{from}, max_sz, from.value);
    }
};

struct FlagsRegister : detail::RegisterBaseImpl<u64>
{
    constexpr bool negative_set() const {
#if defined(ASMGRADER_AARCH64)
        return (nzcv() & NEGATIVE_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
        return (value & SIGN_FLAG_BIT) != 0U;
#endif
    }

    constexpr bool zero_set() const {
#if defined(ASMGRADER_AARCH64)
        return (nzcv() & ZERO_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
        return (value & ZERO_FLAG_BIT) != 0U;
#endif
    }

    constexpr bool carry_set() const {
#if defined(ASMGRADER_AARCH64)
        return (nzcv() & CARRY_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
        return (value & CARRY_FLAG_BIT) != 0U;
#endif
    }

    constexpr bool overflow_set() const {
#if defined(ASMGRADER_AARCH64)
        return (nzcv() & OVERFLOW_FLAG_BIT) != 0U;
#elif defined(ASMGRADER_X86_64)
        return (value & OVERFLOW_FLAG_BIT) != 0U;
#endif
    }

#if defined(ASMGRADER_AARCH64)
    // Specification of pstate (for nzcv) obtained from:
    //   https://developer.arm.com/documentation/ddi0601/2025-06/AArch64-Registers/NZCV--Condition-Flags
    static constexpr u64 NEGATIVE_FLAG_BIT = 0b1000;
    static constexpr u64 ZERO_FLAG_BIT = 0b0100;
    static constexpr u64 CARRY_FLAG_BIT = 0b0010;
    static constexpr u64 OVERFLOW_FLAG_BIT = 0b0001;

    constexpr u64 nzcv() const {
        constexpr u64 NZCV_BIT_OFF = 28;
        constexpr u64 NZCV_BIT_MASK = 0xF;

        return (value >> NZCV_BIT_OFF) & NZCV_BIT_MASK;
    }

#elif defined(ASMGRADER_X86_64)
    // Specification of eflags obtained from:
    //   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
    //   Volume 1 - 3.4.3.1 Status Flags
    static constexpr u64 CARRY_FLAG_BIT = 0b1 << 0;
    static constexpr u64 ZERO_FLAG_BIT = 0b1 << 6;
    static constexpr u64 SIGN_FLAG_BIT = 0b1 << 7;
    static constexpr u64 OVERFLOW_FLAG_BIT = 0b1 << 11;
#endif
};

static_assert(sizeof(FlagsRegister) == sizeof(u64));

template <>
struct fmt::formatter<FlagsRegister> : DebugFormatter
{
    std::size_t bin_labels_offset{};

    constexpr auto parse(fmt::format_parse_context& ctx) {
        const auto* it = DebugFormatter::parse(ctx);
        const auto* end = ctx.end();

        const auto* closing_brace_iter = ranges::find(it, end, '}');

        if (closing_brace_iter == end) {
            return it;
        }

        if (closing_brace_iter - it == 0) {
            return ctx.end();
        }

        if (*it < '0' || *it > '9') {
            throw fmt::format_error("invalid format - offset spec is not an integer");
        }

        int offset_spec = detail::parse_nonnegative_int(it, closing_brace_iter, -1);

        if (offset_spec < 0) {
            throw fmt::format_error("invalid format - offset spec is not a valid integer");
        }

        bin_labels_offset = static_cast<std::size_t>(offset_spec);

        return it;
    }

    static std::string get_short_flags_str(const FlagsRegister& flags) {
        std::vector<char> set_flags;

        if (flags.carry_set()) {
            set_flags.push_back('C');
        }
        if (flags.zero_set()) {
            set_flags.push_back('Z');
        }
        if (flags.negative_set()) {
#if defined(ASMGRADER_AARCH64)
            set_flags.push_back('V');
#elif defined(ASMGRADER_X86_64)
            set_flags.push_back('S');
#endif
        }
        if (flags.overflow_set()) {
            set_flags.push_back('O');
        }

        return fmt::format("{}", fmt::join(set_flags, "|"));
    }

#if defined(ASMGRADER_AARCH64)
    auto format(const FlagsRegister& from, format_context& ctx) const {
        std::string short_flags_str = get_short_flags_str(from);

        if (is_debug_format) {
            // See:
            //   https://developer.arm.com/documentation/ddi0601/2025-06/AArch64-Registers/NZCV--Condition-Flags
            std::string flags_bin = fmt::format("0b{:032b}", from.value);

            // len('0b')
            constexpr auto LABEL_INIT_OFFSET = 2;

            std::string labels_offset(bin_labels_offset + LABEL_INIT_OFFSET, ' ');
            std::string labels = "NZCV";

            ctx.advance_to(format_to(ctx.out(), "{} ({})\n{}{}", flags_bin, short_flags_str, labels_offset, labels));
        } else {
            ctx.advance_to(format_to(ctx.out(), "{}", short_flags_str));
        }

        return ctx.out();
    }
#elif defined(ASMGRADER_X86_64)
    auto format(const FlagsRegister& from, format_context& ctx) const {
        std::string short_flags_str = get_short_flags_str(from);

        if (is_debug_format) {
            // See:
            //   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
            //   Volume 1 - 3.4.3.1 Status Flags
            std::string flags_bin = fmt::format("0b{:032b}", from.value);

            // 32 bits - starting bit# of labels + len('0b')
            constexpr auto LABEL_INIT_OFFSET = 32 - std::bit_width(FlagsRegister::OVERFLOW_FLAG_BIT) + 2;

            std::string labels_offset(bin_labels_offset + LABEL_INIT_OFFSET, ' ');
            std::string labels = "O   SZ     C";

            ctx.advance_to(format_to(ctx.out(), "{} ({})\n{}{}", flags_bin, short_flags_str, labels_offset, labels));
        } else {
            ctx.advance_to(format_to(ctx.out(), "{}", short_flags_str));
        }

        return ctx.out();
    }
#endif
};

struct RegistersState
{
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

    DEF_getter(fp, x29()); // frame pointer
    DEF_getter(lr, x30()); // link register
    u64 sp;                // stack pointer
    u64 pc;                // program counter

    FlagsRegister pstate;

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

    FlagsRegister eflags;

    static constexpr RegistersState from(const user_regs_struct& regs, const user_fpregs_struct& fp_regs);
#endif
};

#if defined(ASMGRADER_AARCH64)
template <>
struct fmt::formatter<RegistersState> : DebugFormatter
{
    auto format(const RegistersState& from, format_context& ctx) const {
        constexpr std::size_t TAB_WIDTH = 4; // for debug format field sep
        std::string field_sep;

        constexpr std::size_t MAX_NAME_LEN = 8; // "x29 [fp]" | "x30 [lr]"

        if (is_debug_format) {
            ctx.advance_to(fmt::format_to(ctx.out(), "RegistersState{{\n{}", std::string(TAB_WIDTH, ' ')));

            field_sep = ",\n" + std::string(TAB_WIDTH, ' ');
        } else {
            ctx.out()++ = '{';

            field_sep = ", ";
        }

        static constexpr auto LEN = [](gsl::czstring str) { return std::string_view{str}.size(); };

        auto print_named_field = [this, &ctx, field_sep, MAX_NAME_LEN](std::string_view name, const auto& what,
                                                                       bool print_sep = true) {
            if (is_debug_format) {
                ctx.advance_to(format_to(ctx.out(), "{:<{}}", name, MAX_NAME_LEN));
            } else {
                ctx.advance_to(format_to(ctx.out(), "{}", name));
            }

            ctx.advance_to(format_to(ctx.out(), " = {}{}", what, (print_sep ? field_sep : "")));
        };

        auto print_reg = [this, print_named_field](std::string_view name, const auto& reg) {
            if (is_debug_format) {
                print_named_field(name, fmt::format("{:?}", reg));
            } else {
                print_named_field(name, fmt::format("{}", reg));
            }
        };

        auto print_uint_field = [this, print_named_field]<std::unsigned_integral UIntType>(std::string_view name,
                                                                                           const UIntType& field) {
            constexpr auto ALIGN = std::numeric_limits<UIntType>::digits10;

            const std::string str = fmt::format("0x{:X}", field);
            if (is_debug_format) {
                print_named_field(name, fmt::format("{:>{}}", str, ALIGN));
            } else {
                print_named_field(name, str);
            }
        };

#define PRINT_gen_reg(z, num, base) print_reg("x" #num, (base).x##num());
        // omit x29 and x30 to do custom formatting
        BOOST_PP_REPEAT(BOOST_PP_SUB(NUM_GEN_REGISTERS, 2), PRINT_gen_reg, from);
#undef PRINT_gen_reg

        print_reg("x29 [fp]", from.fp());
        print_reg("x30 [fp]", from.lr());

#define PRINT_fp_reg(z, num, base) print_reg((is_debug_format ? "q" #num : "d" #num), (base).q##num());
        BOOST_PP_REPEAT(NUM_FP_REGISTERS, PRINT_fp_reg, from);
#undef PRINT_fp_reg

        print_uint_field("fpsr", from.fpsr);
        print_uint_field("fpcr", from.fpcr);

        print_uint_field("sp", from.sp);
        print_uint_field("pc", from.pc);

        if (is_debug_format) {
            constexpr std::size_t PRE_WIDTH = LEN("pstate = ") + TAB_WIDTH;
            const std::string pstate_fmt_spec = fmt::format("{{:?{}}}", PRE_WIDTH);
            print_named_field("pstate", vformat(pstate_fmt_spec, make_format_args(from.pstate)), false);
        } else {
            print_named_field("pstate", from.pstate, false);
        }

        if (is_debug_format) {
            ctx.advance_to(fmt::format_to(ctx.out(), "\n}}"));
        } else {
            ctx.out()++ = '}';
        }

        return ctx.out();
    }
};

constexpr RegistersState RegistersState::from(const user_regs_struct& regs, const user_fpsimd_struct& fpsimd_regs) {
    RegistersState result{};

    ranges::copy(regs.regs | ranges::views::transform([](auto value) { return GeneralRegister{value}; }),
                 result.regs.begin());

    result.sp = regs.sp;
    result.pc = regs.pc;
    result.pstate.value = regs.pstate;

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
    check_reg(before_call.sp, after_call.sp, "sp");

    if (err_changed_names.empty()) {
        return {};
    }

    return err_changed_names;
}

#elif defined(ASMGRADER_X86_64)

template <>
struct fmt::formatter<RegistersState> : DebugFormatter
{
    auto format(const RegistersState& from, format_context& ctx) const {
        constexpr std::size_t TAB_WIDTH = 4; // for debug format field sep
        std::string field_sep;

        constexpr std::size_t MAX_NAME_LEN = 6; // "eflags"

        if (is_debug_format) {
            ctx.advance_to(fmt::format_to(ctx.out(), "RegistersState{{\n{}", std::string(TAB_WIDTH, ' ')));

            field_sep = ",\n" + std::string(TAB_WIDTH, ' ');
        } else {
            ctx.out()++ = '{';

            field_sep = ", ";
        }

        static constexpr auto LEN = [](gsl::czstring str) { return std::string_view{str}.size(); };

        auto print_named_field = [this, &ctx, field_sep, MAX_NAME_LEN](std::string_view name, const auto& what,
                                                                       bool print_sep = true) {
            if (is_debug_format) {
                ctx.advance_to(format_to(ctx.out(), "{:<{}}", name, MAX_NAME_LEN));
            } else {
                ctx.advance_to(format_to(ctx.out(), "{}", name));
            }

            ctx.advance_to(format_to(ctx.out(), " = {}{}", what, (print_sep ? field_sep : "")));
        };

        auto print_gen_reg = [this, print_named_field](std::string_view name, const auto& reg) {
            if (is_debug_format) {
                print_named_field(name, fmt::format("{:?}", reg));
            } else {
                print_named_field(name, fmt::format("{}", reg));
            }
        };

        auto print_u64_field = [this, print_named_field](std::string_view name, const u64& field) {
            constexpr auto ALIGN = std::numeric_limits<u64>::digits10;

            const std::string str = fmt::format("0x{:X}", field);
            if (is_debug_format) {
                print_named_field(name, fmt::format("{:>{}}", str, ALIGN));
            } else {
                print_named_field(name, str);
            }
        };

#define PRINT_gen_reg(r, base, elem) print_gen_reg(BOOST_PP_STRINGIZE(elem), (base).elem);

        BOOST_PP_SEQ_FOR_EACH(PRINT_gen_reg, from, REGS_tuple_set);
#undef PRINT_gen_reg

        print_u64_field("rip", from.rip);
        print_u64_field("rsp", from.rip);

        if (is_debug_format) {
            constexpr std::size_t PRE_WIDTH = LEN("eflags = ") + TAB_WIDTH;
            const std::string eflags_fmt_spec = fmt::format("{{:?{}}}", PRE_WIDTH);
            print_named_field("eflags", vformat(eflags_fmt_spec, make_format_args(from.eflags)), false);
        } else {
            print_named_field("eflags", from.eflags, false);
        }

        if (is_debug_format) {
            ctx.advance_to(fmt::format_to(ctx.out(), "\n}}"));
        } else {
            ctx.out()++ = '}';
        }

        return ctx.out();
    }
};

// A benefit of using the same names as user_regs_struct
#define COPY_gen_reg(r, base, elem) base.elem.value = regs.elem;

constexpr RegistersState RegistersState::from(const user_regs_struct& regs, const user_fpregs_struct& fp_regs) {
    RegistersState result{};

    BOOST_PP_SEQ_FOR_EACH(COPY_gen_reg, result, REGS_tuple_set);

    result.rip = regs.rip;
    result.rsp = regs.rsp;

    result.eflags.value = regs.eflags;

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
