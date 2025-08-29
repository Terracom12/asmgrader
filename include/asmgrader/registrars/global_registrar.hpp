#pragma once

#include <asmgrader/api/assignment.hpp>
#include <asmgrader/api/metadata.hpp>
#include <asmgrader/api/test_base.hpp>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/transform.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace asmgrader {

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

    /// Registers the test to be made accessible
    void add_test(std::unique_ptr<TestBase> test) noexcept;

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

    /// Find the assignment corresponding to the `Assignment` attribute of metadata,
    /// or create a new assignment if one does not exist.
    template <typename... MetadataAttrs>
    Assignment& find_or_create_assignment(metadata::Metadata<MetadataAttrs...> metadata);

    /// Obtain a list of all assignment names
    std::vector<std::string_view> get_assignment_names();

    std::size_t get_num_registered() const;

private:
    GlobalRegistrar() = default;

    /// Obtain an assignment for the given test name, or create a new one if nonexistant
    Assignment& get_assignment_for_test(const TestBase& base);

    std::vector<std::unique_ptr<Assignment>> registered_assignments_;
};

template <typename... MetadataAttrs>
Assignment& GlobalRegistrar::find_or_create_assignment(metadata::Metadata<MetadataAttrs...> metadata) {
    static_assert(metadata.template has<metadata::Assignment>(), "metadata::Assignment attribute missing");

    metadata::Assignment meta_assignment = *metadata.template get<metadata::Assignment>();

    auto assignment_matcher = [meta_assignment](const std::unique_ptr<Assignment>& assignment) {
        return assignment->get_name() == meta_assignment.name &&
               assignment->get_exec_path() == meta_assignment.exec_name;
    };

    if (auto iter = ranges::find_if(registered_assignments_, assignment_matcher);
        iter != registered_assignments_.end()) {
        return **iter;
    }

    registered_assignments_.push_back(std::make_unique<Assignment>(meta_assignment.name, meta_assignment.exec_name));

    return *registered_assignments_.back();
}

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

} // namespace asmgrader
