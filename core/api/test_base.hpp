#pragma once

#include "api/metadata.hpp"
#include "test_context.hpp"

#include <optional>
#include <string_view>

class Assignment;

/// Base class primarily for a user-written test
///
/// Provides member functions for any test action the user might want to perform within a test case.
class TestBase
{
public:
    template <typename... MetadataAttrs>
    explicit TestBase(const Assignment& assignment, std::string_view name,
                      metadata::Metadata<MetadataAttrs...> metadata = metadata::Metadata<MetadataAttrs...>{}) noexcept
        : name_{name}
        , assignment_{&assignment}
        , is_prof_only_{metadata.template get<metadata::ProfOnlyTag>()}
        , weight_{metadata.template get<metadata::Weight>()} {}

    virtual ~TestBase() noexcept = default;

    virtual void run(TestContext& ctx) = 0;

    const Assignment& get_assignment() const { return *assignment_; }

    std::string_view get_name() const noexcept { return name_; }

    bool get_is_prof_only() const noexcept { return is_prof_only_; }

    std::optional<metadata::Weight> get_weight() const noexcept { return weight_; }

private:
    std::string_view name_;
    const Assignment* assignment_;

    // TODO: Probably want a more versatile way to handle metadata, but this should suffice for now
    // Name could also be considered metadata...
    bool is_prof_only_;
    std::optional<metadata::Weight> weight_;
};
