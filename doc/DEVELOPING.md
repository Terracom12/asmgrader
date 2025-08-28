# Library Developer Guide

## Project Structure

```
doc/                    - Documentation sources and config

hooks/                  - Git hooks; install locally by running scripts/install-hooks.sh

scripts/                - Utility scripts

cmake/                  - CMake utility functions

include/asmgrader/
    api/                - High-level API for writing student tests
    common/             - Various utilities
        formatters/     - fmt::formatter specializations and utilities
    meta/               - Template metaprogramming utilities
    program/            - Wrapper around a subprocess
    registrars/         - Global registration for student tests and assignments
    subprocess/         - Hand-crafted subprocess and tracer implementations
        memory/         - API for interacting with subprocess memory
    symbols/            - (ELF only) Symbol loading and address resolution
    asmgrader.hpp       - Main project header

src/
    app/                - Top-level application functionality
    output/             - 
    user/               - End user input (CLI args; file searching)

    ... [Directories also present in include/ are omitted] ...

test/                   - Internal tests (for the library itself, not user/student code)

cs3b-grader/            - Private submodule; Assignment/lab "graders" specifically for the CS3B course

```

