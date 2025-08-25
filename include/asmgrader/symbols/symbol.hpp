#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

#include <cctype>
#include <cstddef>
#include <string>

namespace asmgrader {

/// Basic definition for a symbol found in an ELF file
struct Symbol
{
    std::string name;

    /// I.e.: found in .symtab or .dynsym respectively
    enum { Static, Dynamic } kind;

    std::size_t address;

    enum { Local, Global, Weak, Other } binding;
};

} // namespace asmgrader

template <>
struct fmt::formatter<::asmgrader::Symbol> : formatter<std::string>
{
    template <typename Context>
    auto format(const ::asmgrader::Symbol& from, Context& ctx) const {
        using Symbol = ::asmgrader::Symbol;

        const std::string kind_str = [&from]() {
            switch (from.kind) {
            case Symbol::Static:
                return "Static";
            case Symbol::Dynamic:
                return "Dynamic";
            default:
                return "<unknown>";
            }
        }();
        const std::string binding_str = [&from]() {
            switch (from.binding) {
            case Symbol::Local:
                return "Local";
            case Symbol::Global:
                return "Global";
            case Symbol::Weak:
                return "Weak";
            case Symbol::Other:
                return "Other";
            default:
                return "<unknown>";
            }
        }();

        return fmt::format_to(ctx.out(), "Symbol{{.name={:?}, .kind={}, .address=0x{:X}, .binding={}}}", from.name,
                              kind_str, from.address, binding_str);
    }
};
