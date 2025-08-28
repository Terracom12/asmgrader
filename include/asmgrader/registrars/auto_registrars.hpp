#pragma once

#include <asmgrader/api/assignment.hpp>
#include <asmgrader/api/metadata.hpp>
#include <asmgrader/api/test_base.hpp>
#include <asmgrader/registrars/global_registrar.hpp>

#include <concepts>
#include <memory>
#include <string_view>

namespace asmgrader {

// /// Helper class that, whem constructed, automatically registers an assignment to the global registrar
// class AssignmentAutoRegistrar
// {
// public:
//     explicit AssignmentAutoRegistrar(Assignment assignment) noexcept {
//         GlobalRegistrar::get().add(std::move(assignment));
//     }
// };

/// Helper class that, whem constructed, automatically constructs and registers a test
template <typename TestClass>
    requires(std::derived_from<TestClass, TestBase>)
class TestAutoRegistrar
{
public:
    template <typename... MetadataAttrs>
    explicit TestAutoRegistrar(std::string_view name, metadata::Metadata<MetadataAttrs...> metadata =
                                                          metadata::Metadata<MetadataAttrs...>{}) noexcept {

        Assignment& assignment = GlobalRegistrar::get().find_or_create_assignment(metadata);

        TestClass test{assignment, name, metadata};
        assignment.add_test(std::make_unique<TestClass>(std::forward<TestClass>(test)));
    }
};

} // namespace asmgrader
