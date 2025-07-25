# CS3B Autograder

An application that automatically grades students' labs for Prof. Eaton's CS3B course at Saddleback College.

## Dependencies

```console
sudo apt install cmake libboost-all-dev libelf-dev
```

## Project Structure

```
core/
    action/             - DEPRECATED
    meta/               - Template metaprogramming utilities
    program/            - 
    registrars/
    subprocess/
    symbols/            - (ELF only) Symbol loading + address resolution
    test/               - 
    user/               - End user input and output (CLI)
    util/               - General utilities

test/                   - Project unit tests

autograder/             - RENAME; Assignment/lab "graders"

cmake/                  - CPM downloader + Cross-compilation rules (- soon)
```
