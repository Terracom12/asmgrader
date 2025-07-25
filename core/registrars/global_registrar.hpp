#pragma once

#include "test/assignment.hpp"

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

/// A global singleton registrar. Used with ``Assignment``s for now.
///
/// Construct this class with the assignment we want to register,
/// and that assignement will be made accessible at the CLI-level,
/// premitting the user to run the grader on that assignment.
///
/// `add` might be better named `register` if not for the fact for it being
/// a reserved keyword.
class GlobalRegistrar
{
public:
    /// Safe global singleton pattern (first intro. by Scott Meyers for C++, I think)
    static GlobalRegistrar& get() noexcept;

    /// Registers the assignment to be made accessible
    Assignment& add(Assignment assignment) noexcept;

    template <typename Func>
        requires(std::is_void_v<std::invoke_result_t<Func, Assignment>>)
    void for_each_assignment(Func&& fun);

    template <typename Func>
        requires(!std::is_void_v<std::invoke_result_t<Func, Assignment>>)
    std::vector<std::invoke_result_t<Func, Assignment>> for_each_assignment(Func&& fun);

    auto get_assignments() noexcept {
        return registered_assignments_ |
               ranges::views::transform(
                   [](std::unique_ptr<Assignment>& assignment) -> Assignment& { return *assignment; });
    }

    std::optional<std::reference_wrapper<Assignment>> get_assignment(std::string_view name) {
        auto name_matcher = [name](const std::unique_ptr<Assignment>& assignment) {
            return assignment->get_name() == name;
        };

        if (auto iter = ranges::find_if(registered_assignments_, name_matcher); iter != registered_assignments_.end()) {
            return **iter;
        }

        return std::nullopt;
    }

    /// Obtain a list of all assignment names
    std::vector<std::string_view> get_assignment_names();

    std::size_t get_num_registered() const;

private:
    GlobalRegistrar() = default;

    std::vector<std::unique_ptr<Assignment>> registered_assignments_;
};

template <typename Func>
    requires(std::is_void_v<std::invoke_result_t<Func, Assignment>>)
void GlobalRegistrar::for_each_assignment(Func&& fun) {
    for (auto& assignement : get_assignments()) {
        std::forward<Func>(fun)(assignement);
    }
}

template <typename Func>
    requires(!std::is_void_v<std::invoke_result_t<Func, Assignment>>)
std::vector<std::invoke_result_t<Func, Assignment>> GlobalRegistrar::for_each_assignment(Func&& fun) {
    std::vector<std::invoke_result_t<Func, Assignment>> result;
    result.reserve(registered_assignments_.size());

    for (auto& assignement : get_assignments()) {
        result.push_back(std::forward<Func>(fun)(assignement));
    }

    return result;
}
