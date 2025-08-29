#include "registrars/global_registrar.hpp"

#include "api/assignment.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace asmgrader {

GlobalRegistrar& GlobalRegistrar::get() noexcept {
    // thread-safe singleton initialization pattern
    static GlobalRegistrar local_instance{};

    return local_instance;
}

// void GlobalRegistrar::add_test(std::unique_ptr<TestBase> test) noexcept {
//     registered_assignments_.push_back(std::make_unique<Assignment>(std::move(assignment)));
//
//     return *registered_assignments_.back().get();
// }

std::vector<std::string_view> GlobalRegistrar::get_assignment_names() {
    return for_each_assignment([](const Assignment& assignment) { return assignment.get_name(); });
}

std::size_t GlobalRegistrar::get_num_registered() const {
    return registered_assignments_.size();
}

} // namespace asmgrader
