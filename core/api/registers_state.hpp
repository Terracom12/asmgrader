#pragma once

#include "common/aliases.hpp"
#include "common/expected.hpp"
#include "common/formatters/debug.hpp"
#include "common/os.hpp"
#include "logging.hpp"
#include "meta/integer.hpp"

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
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
#include <range/v3/view/map.hpp>
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
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <sys/user.h> // user_regs_struct, ([x86_64] user_fpregs_struct | [aarch64] user_fpsimd_struct)

/// All header only as this is relatively low level and we want operations to be fast (inlinable)

namespace detail {

/// CRTP (or is it CRTTP in this case?) is used to pass an arch alternative.
/// See gh#21 for details.
template <template <ProcessorKind> typename Derived, typename BaseType, ProcessorKind Arch>
struct RegisterBaseImpl;

template <template <ProcessorKind> typename Derived, typename BaseType, ProcessorKind Arch>
    requires(Arch == SYSTEM_PROCESSOR)
struct RegisterBaseImpl<Derived, BaseType, Arch>
{
    constexpr RegisterBaseImpl() = default;

    template <ProcessorKind OtherArch>
    constexpr explicit RegisterBaseImpl(const Derived<OtherArch>* /*arch_alternative*/) {}

    constexpr explicit RegisterBaseImpl(BaseType value)
        : value_{value} {}

    constexpr auto& operator=(BaseType rhs) {
        value_ = rhs;
        return *static_cast<Derived<Arch>*>(this);
    }

    constexpr BaseType& get_value() { return value_; }

    constexpr const BaseType& get_value() const { return value_; }

    template <std::integral IntType>
    constexpr IntType as() const {
        // FIXME: Something something endianness

        if (std::is_constant_evaluated()) {
            // Set all bits (either 2's complement, or undeflow of unsigned integer)
            constexpr auto ALL_BITS = static_cast<IntType>(-1);

            // NOLINTNEXTLINE(bugprone-signed-char-misuse,cert-str34-c)
            constexpr auto MASK = static_cast<u64>(ALL_BITS);

            return static_cast<IntType>(MASK & value_);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const IntType*>(&value_);
    }

    constexpr auto operator<=>(const RegisterBaseImpl<Derived, BaseType, Arch>& rhs) const = default;

private:
    BaseType value_{};
};

template <template <ProcessorKind> typename Derived, typename BaseType, ProcessorKind Arch>
    requires(Arch != SYSTEM_PROCESSOR)
struct RegisterBaseImpl<Derived, BaseType, Arch>
{
    constexpr RegisterBaseImpl() = default;

    template <ProcessorKind OtherArch>
    constexpr explicit RegisterBaseImpl(Derived<OtherArch>* arch_alternative)
        : alternative{arch_alternative} {}

    constexpr explicit RegisterBaseImpl(BaseType /*value*/) {}

    constexpr auto& operator=(BaseType rhs) {
        get_value() = rhs;
        return *this;
    }

    /// This isn't exactly a general solution, but I don't think that this library will
    /// ever support more than 2 architectures
    Derived<SYSTEM_PROCESSOR>* alternative{};

    void check_log_alternative() const {
        if (alternative == nullptr) {
            constexpr auto MSG = "Attempting to access non-native register without a native alternative";
            LOG_ERROR(MSG);
            throw std::runtime_error(MSG);
        }

        LOG_WARN("Attempting to access non-native register. Defaulting to a native alternative");
    }

    constexpr decltype(auto) get_value() const {
        check_log_alternative();
        return alternative->get_value();
    }

    template <std::integral IntType>
    constexpr IntType as() const {
        check_log_alternative();
        return alternative->template as<IntType>();
    }

    constexpr auto operator<=>(const RegisterBaseImpl<Derived, BaseType, Arch>& rhs) const = default;
};

} // namespace detail

/// General Purpose Register
template <ProcessorKind Arch = SYSTEM_PROCESSOR>
struct IntRegister : detail::RegisterBaseImpl<IntRegister, u64, Arch>
{
    using detail::RegisterBaseImpl<IntRegister, u64, Arch>::RegisterBaseImpl;
    using detail::RegisterBaseImpl<IntRegister, u64, Arch>::operator=;

    // NOLINTNEXTLINE(google-explicit-constructor)
    /*implicit*/ constexpr operator u64() const {
        return detail::RegisterBaseImpl<IntRegister, u64, Arch>::get_value();
    }
};

static_assert(sizeof(IntRegister<>) == sizeof(u64));

template <ProcessorKind Arch>
struct fmt::formatter<IntRegister<Arch>> : DebugFormatter
{
    auto format(const IntRegister<Arch>& from, format_context& ctx) const {
        if (is_debug_format) {
            using IntType = decltype(from.get_value());
            constexpr auto ALIGN_10 = meta::digits10_max_count<IntType>;
            constexpr auto ALIGN_16 = sizeof(IntType) * 2;
            constexpr auto ALIGN_2 = sizeof(IntType) * 8;
            return format_to(ctx.out(), "{0:>{1}} | 0x{0:0{2}X} | 0b{0:0{3}B}", from.get_value(), ALIGN_10, ALIGN_16,
                             ALIGN_2);
        }

        return format_to(ctx.out(), "{}", from.get_value());
    }
};

template <ProcessorKind Arch = SYSTEM_PROCESSOR>
struct FloatingPointRegister : detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>
{
    using detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>::RegisterBaseImpl;
    using detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>::operator=;

    // FIXME: floating point trait that supports f128
    template </*std::floating_point*/ typename FPType>
        requires(Arch == SYSTEM_PROCESSOR)
    static constexpr FloatingPointRegister from(FPType val) {
        using IntType = meta::sized_uint_t<sizeof(FPType)>;
        auto int_val = std::bit_cast<IntType>(val);

        return FloatingPointRegister{int_val};
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    /*implicit*/ constexpr operator double() const {
        return std::bit_cast<double>(
            static_cast<u64>(detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>::get_value()));
    }

    using detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>::as;

    template <std::floating_point FPType>
    constexpr FPType as() const {
        using IntType = meta::sized_uint_t<sizeof(FPType)>;
        auto int_val = detail::RegisterBaseImpl<FloatingPointRegister, u128, Arch>::template as<IntType>();

        return std::bit_cast<FPType>(int_val);
    }
};

static_assert(sizeof(FloatingPointRegister<>) == sizeof(u128));

template <ProcessorKind Arch>
struct fmt::formatter<FloatingPointRegister<Arch>> : DebugFormatter
{
    auto format(const FloatingPointRegister<Arch>& from, format_context& ctx) const {
        if (!is_debug_format) {
            return format_to(ctx.out(), "{}", double{from});
        }

        const auto max_sz = formatted_size("{}", std::numeric_limits<double>::min());
        return format_to(ctx.out(), "{:>{}} (f64) | 0x{:032X}", double{from}, max_sz, from.get_value());
    }
};

template <ProcessorKind Arch = SYSTEM_PROCESSOR>
struct FlagsRegister : detail::RegisterBaseImpl<FlagsRegister, u64, Arch>
{
    using detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::RegisterBaseImpl;
    using detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::operator=;

    constexpr bool negative_set() const {
        return (detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::get_value() & NEGATIVE_FLAG_BIT) != 0U;
    }

    constexpr bool zero_set() const {
        return (detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::get_value() & ZERO_FLAG_BIT) != 0U;
    }

    constexpr bool carry_set() const {
        return (detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::get_value() & CARRY_FLAG_BIT) != 0U;
    }

    constexpr bool overflow_set() const {
        return (detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::get_value() & OVERFLOW_FLAG_BIT) != 0U;
    }

#if defined(ASMGRADER_AARCH64)
    // Specification of pstate (for nzcv) obtained from:
    //   https://developer.arm.com/documentation/ddi0601/2025-06/AArch64-Registers/NZCV--Condition-Flags
    static constexpr u64 NZCV_BASE_OFF = 28;
    static constexpr u64 NEGATIVE_FLAG_BIT = 1U << (NZCV_BASE_OFF + 3);
    static constexpr u64 ZERO_FLAG_BIT = 1U << (NZCV_BASE_OFF + 2);
    static constexpr u64 CARRY_FLAG_BIT = 1U << (NZCV_BASE_OFF + 1);
    static constexpr u64 OVERFLOW_FLAG_BIT = 1U << (NZCV_BASE_OFF + 0);

    constexpr u64 nzcv() const {
        constexpr u64 NZCV_BIT_MASK = 0xF;

        return (detail::RegisterBaseImpl<FlagsRegister, u64, Arch>::get_value() >> NZCV_BASE_OFF) & NZCV_BIT_MASK;
    }

#elif defined(ASMGRADER_X86_64)
    // Specification of eflags obtained from:
    //   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
    //   Volume 1 - 3.4.3.1 Status Flags
    static constexpr u64 CARRY_FLAG_BIT = 1U << 0;
    static constexpr u64 ZERO_FLAG_BIT = 1U << 6;
    static constexpr u64 NEGATIVE_FLAG_BIT = 1U << 7;
    static constexpr u64 OVERFLOW_FLAG_BIT = 1U << 11;
#endif
};

static_assert(sizeof(FlagsRegister<>) == sizeof(u64));

template <ProcessorKind Arch>
struct fmt::formatter<FlagsRegister<Arch>> : DebugFormatter
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

    static std::string get_short_flags_str(const FlagsRegister<Arch>& flags) {
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
    auto format(const FlagsRegister<Arch>& from, format_context& ctx) const {
        std::string short_flags_str = get_short_flags_str(from);

        if (is_debug_format) {
            // See:
            //   https://developer.arm.com/documentation/ddi0601/2025-06/AArch64-Registers/NZCV--Condition-Flags
            std::string flags_bin = fmt::format("0b{:032b}", from.get_value());

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
    auto format(const FlagsRegister<Arch>& from, format_context& ctx) const {
        std::string short_flags_str = get_short_flags_str(from);

        if (is_debug_format) {
            // See:
            //   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
            //   Volume 1 - 3.4.3.1 Status Flags
            std::string flags_bin = fmt::format("0b{:032b}", from.get_value());

            // 32 bits - starting bit# of labels + len('0b')
            constexpr auto LABEL_INIT_OFFSET = 32 - std::bit_width(FlagsRegister<Arch>::OVERFLOW_FLAG_BIT) + 2;

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
    //
    // ##################### Aarch64
    //
#define DEF_getter(name, expr)                                                                                         \
    constexpr auto name() const { return expr; }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NUM_GEN_REGISTERS 31
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NUM_FP_REGISTERS 32

    // General registers (x0-x30)
    std::array<IntRegister<ProcessorKind::Aarch64>, NUM_GEN_REGISTERS> regs;

#define DEF_gen_register(z, num, _) DEF_getter(x##num, regs[num]) DEF_getter(w##num, static_cast<u32>(regs[num]))
    BOOST_PP_REPEAT(NUM_GEN_REGISTERS, DEF_gen_register, ~);
#undef DEF_gen_register

    DEF_getter(fp, x29());                  // frame pointer
    DEF_getter(lr, x30());                  // link register
    IntRegister<ProcessorKind::Aarch64> sp; // stack pointer
    IntRegister<ProcessorKind::Aarch64> pc; // program counter

    FlagsRegister<ProcessorKind::Aarch64> pstate{&eflags};

    // Floating-point registers (Q0-Q31)
    std::array<FloatingPointRegister<ProcessorKind::Aarch64>, NUM_FP_REGISTERS> vregs;

#define DEF_fp_register(z, num, _)                                                                                     \
    DEF_getter(q##num, vregs[num]) DEF_getter(d##num, vregs[num].as<u64>()) DEF_getter(s##num, vregs[num].as<u32>())   \
        DEF_getter(h##num, vregs[num].as<u16>())
    BOOST_PP_REPEAT(NUM_FP_REGISTERS, DEF_fp_register, ~);
#undef DEF_fp_register

    /// Actually a u32, but I can't imagine a scenario where that would matter,
    /// so I don't think it's worth a couple more class templates.
    IntRegister<ProcessorKind::Aarch64> fpsr; // Floating-point status register

    /// Actually a u32, but I can't imagine a scenario where that would matter,
    /// so I don't think it's worth a couple more class templates.
    IntRegister<ProcessorKind::Aarch64> fpcr; // Floating-point control register

#undef DEF_getter

#ifdef ASMGRADER_AARCH64
    static constexpr RegistersState from(const user_regs_struct& regs, const user_fpsimd_struct& fpsimd_regs);
#endif

    //
    // ##################### x86_64
    //

    // TODO: Different access modes for general registers (eax, ax, al, ah, etc.)
#define REGS_tuple_set                                                                                                 \
    BOOST_PP_TUPLE_TO_SEQ(15, (r15, r14, r13, r12, rbp, rbx, r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi))
#define DEF_gen_register(r, _, elem) IntRegister<ProcessorKind::x86_64> elem;
    BOOST_PP_SEQ_FOR_EACH(DEF_gen_register, _, REGS_tuple_set)
#undef DEF_gen_register

    IntRegister<ProcessorKind::x86_64> rip; // instruction pointer
    IntRegister<ProcessorKind::x86_64> rsp; // stack pointer

    FlagsRegister<ProcessorKind::x86_64> eflags;

#ifdef ASMGRADER_X86_64
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

#define PRINT_gen_reg(z, num, base) print_reg("x" #num, (base).x##num());
        // omit x29 and x30 to do custom formatting
        BOOST_PP_REPEAT(BOOST_PP_SUB(NUM_GEN_REGISTERS, 2), PRINT_gen_reg, from);
#undef PRINT_gen_reg

        print_reg("x29 [fp]", from.fp());
        print_reg("x30 [lr]", from.lr());

#define PRINT_fp_reg(z, num, base) print_reg((is_debug_format ? "q" #num : "d" #num), (base).q##num());
        BOOST_PP_REPEAT(NUM_FP_REGISTERS, PRINT_fp_reg, from);
#undef PRINT_fp_reg

        print_reg("fpsr", from.fpsr);
        print_reg("fpcr", from.fpcr);

        print_reg("sp", from.sp);
        print_reg("pc", from.pc);

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

    auto writable_regs_view =
        result.regs | ranges::views::transform([](auto& reg) -> decltype(auto) { return reg.get_value(); });
    ranges::copy(regs.regs, ranges::begin(writable_regs_view));

    result.sp = regs.sp;
    result.pc = regs.pc;
    result.pstate.get_value() = regs.pstate;

    auto writable_vregs_view =
        result.vregs | ranges::views::transform([](auto& reg) -> decltype(auto) { return reg.get_value(); });
    ranges::copy(fpsimd_regs.vregs, ranges::begin(writable_vregs_view));

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

        auto print_reg = [this, print_named_field](std::string_view name, const auto& reg) {
            if (is_debug_format) {
                print_named_field(name, fmt::format("{:?}", reg));
            } else {
                print_named_field(name, fmt::format("{}", reg));
            }
        };

#define PRINT_gen_reg(r, base, elem) print_reg(BOOST_PP_STRINGIZE(elem), (base).elem);

        BOOST_PP_SEQ_FOR_EACH(PRINT_gen_reg, from, REGS_tuple_set);
#undef PRINT_gen_reg

        print_reg("rip", from.rip);
        print_reg("rsp", from.rip);

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

constexpr RegistersState RegistersState::from(const user_regs_struct& regs, const user_fpregs_struct& fp_regs) {
#define COPY_gen_reg(r, base, elem) base.elem.get_value() = regs.elem;
    RegistersState result{};

    BOOST_PP_SEQ_FOR_EACH(COPY_gen_reg, result, REGS_tuple_set);

    result.rip = regs.rip;
    result.rsp = regs.rsp;

    result.eflags.get_value() = regs.eflags;

    // TODO: Support floating point on x86_64
    std::ignore = fp_regs;

    return result;
#undef COPY_gen_reg
}

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
