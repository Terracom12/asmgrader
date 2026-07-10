- [x] function arg repr via `str()`
- [x] pass an `expected<>` type to asm function; custom msg if an error, otherwise unwrap
- [x] wrapper type for `str` and `repr` results to not wrongfully stringize
- [ ] Wrapper types for various items as to display properly
  - addresses
  - register values
  - register flags
  - ...
- [x] lab5-1 impl (GDB stories)
  - [x] `run_to_exit()` (naming??)
- [ ] more informative test case name display when verbosity < Extra
- [ ] Non-shitty color scheme
- [x] fixed canvas filename spaces issue
- [ ] docs for database
- [ ] Default DEBUG && TRACE logs; disabled INTENTIONALLY (ASMGRADER_NDEBUG or smth)
- [ ] Fix include directory detection for header files
**
- [ ] Have a logging fd for child proc that connects directly to parents proc's stderr
**
- [ ] DO THIS FRICKING THING:
  - `chromium-browser --headless --disable-gpu --screenshot=google.png --window-size=1280,1920 http://www.google.com`

## Cross-compilation
`make build CROSS_COMPILE=1 CMAKE_CONFIGURE_EXTRA_ARGS='-DCMAKE_CROSSCOMPILING_EMULATOR="qemu-aarch64;-L;/usr/aarch64-linux-gnu/"'`
`docker run -v ${CPM_SOURCE_CACHE}:/workspace/CPM:Z -v "$(ccache -p | sed -En 's/^.*cache_dir =\s+(.*)/\1/p')":/workspace/ccache:Z -v ./build:/workspace/build:Z --rm aarch64-cross make build CROSS_COMPILE=1`

- [ ] DISREGARD version info for pre-push hook. Fucking annoying

- [ ] Timeout should not kill a test!

## Semantic Requirements
- [ ] Implement naive versions with non-nice output
  - REQUIRE_FALSE
  - REQUIRE_MATCHES
  - REQUIRE_\[NON\]EMPTY
  - REQUIRE_CONTAINS
    - both for actual items (like vector<int>{}.contains(3)) and string searching, like string{"abcdef"}.contains("bc")
- [ ] Semantically pleasant output
- [ ] Semantic file requirements
- [ ] Semantic syscall requirements


## 10/20/2025
- [x] Fix timeout bug (30 mins max)
- [x] Add TempFile API
- [ ] Create Lab7-1 tests
- [ ] Add html output (maybe)

## BUGS BUGS BUGS
- [ ] Broken verbose serialization (see lab7-2; `==` sign weirdness)
