#pragma once

#include "app/trace_exception.hpp"
#include "common/class_traits.hpp"
#include "user/program_options.hpp"

#include <optional>
#include <utility>

namespace asmgrader {

class App : NonCopyable
{
public:
    explicit App(ProgramOptions opts)
        : OPTS{std::move(opts)} {}

    virtual ~App() = default;

    const ProgramOptions& get_opts() const noexcept { return OPTS; }

    int run() noexcept {
        std::optional res = wrap_throwable_fn(&App::run_impl, this);

        return res.value_or(-1);
    }

    const ProgramOptions OPTS;

protected:
    virtual int run_impl() = 0;
};

} // namespace asmgrader
