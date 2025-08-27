## Add Libraries
- [ ] constexpr containers support
  - [ ] optional lib (reference impl one, I think)
- [ ] libassert :D
- [ ] magicenum or that faster one by the libassert guy


- [ ] Ensure working on clean pi 4b and 5

## docs

- [ ] Expand README
- [ ] Doxygen builds
- [ ] copyright headers

## COOL Features

- [ ] GDB break mode for failing req

## core

Tasks for core functionality

### Input

- [ ] Handle empty executable

### REFACTORS

- [ ] Tagged variant or polymorphic (latter probably better) ErrorKind for more info
- [ ] Test Metadata: {must be compile time objects}
        assignment(name, filename)
        professor only
        weight (idk how to design yet)
  - [ ] File default
  - [ ] Combine with custom options
  - [ ] Combine with structure

## Design

System design (mainly of high-level items)

- [ ] TestRequirement design
- [ ] AsmFunction and like end-user API linkage to functional implementations
- [ ] Plain-text output
- [ ] Log redirection and suppression
- [ ] Output API
  - Passing/Failing tests
  - Grade
  - Assignments
  - Extra info for failing tests
  - Various customization points (info above is not "baked-in" to output renderers)
- [ ] Professor back-end -- grade all matching one assignment; enable more tests

### IDEAS

Not really TODO stuff, but just potential ideas for the section above

#### `TestBase` ideas
- `TestBase` can *do* many different relevant actions (jump to address, call a function, set regs & memory, etc.) and also can output results via a plethora of public member functions.
  - High-level adaptors (e.g., `AsmFunction`) simply use these public functions to implement behavior
  - `TestRequirement` uses the output handling functions of `TestBase`

#### Output ideas
- Classes for sinks (files / other destinations?) and serializers (plaintext, json, html?)
- Global registrar stores pairs of serializer+sink references for means of output
  - Based on CLI options and/or heuristics (don't send color if redirected, plaintext for students, etc.)
- Buffer output until tests are complete or an exception occurs
- Generic abstract functions for OutputSerializers
  -

## (AI-generated) Quick MVP tasks

### 1. Test Execution Path
- [x] Finalize `TEST(...)` macro expansion → `TestBase` subclass with `run_test()` defined.
- [x] Ensure each test is registered in the global registrar at startup.
- [ ] Implement a simple test runner loop that:
  - [x] Iterates through registered tests (filtered by CLI selection).
  - [x] Runs each test synchronously.
  - [ ] Catches assertion failures and unexpected exceptions.

- [ ] Implement assignment/test naming scheme for listing and selection.

- [ ] Implement optional `setup()` and `teardown()` hooks
- [ ] Add result mutation/logging API (e.g. `fail()`, `pass()`, `log()`)

## Optional Refinements (Only if Time Permits)
- [ ] Rename `TracedSubprocess` to reflect responsibilities (e.g. `ExecutableInstance`).
- [ ] Add test name filtering (e.g. regex).
- [ ] Buffer output and flush after each test (for future JSON/HTML).
- [ ] Include exception details in test failure output.
