# AsmGrader

An application that automatically grades students' labs for Prof. Eaton's CS3B course at Saddleback College.

## Project Structure

```
core/
    meta/               - Template metaprogramming utilities
    program/
    registrars/
    subprocess/
    symbols/            - (ELF only) Symbol loading + address resolution
    test/               - 
    user/               - End user input and output (CLI)
    util/               - General utilities

test/                   - Project unit tests

cs3b-grader/             - Submodule; Assignment/lab "graders"

cmake/                  - CPM downloader + Cross-compilation rules (- soon)
```
