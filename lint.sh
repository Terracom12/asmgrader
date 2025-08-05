#!/bin/bash

# Exit on error
set -e

ROOT_DIR=$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd -P
)

cd "$ROOT_DIR"

lint_files=$(
    find core/ grader/ tests/ -name '*.hpp' -or -name '*.cpp'
)
compilation_db=build/compile_commands.json

clang-tidy $lint_files -p "$compilation_db" --warnings-as-errors='*'
clang-format $lint_files --dry-run -Werror

# Find improper library header conventions ("" instead of <>)
grep -E '"(range|boost|fmt|catch2|nlohmann|argparse|gsl)/' $lint_files
