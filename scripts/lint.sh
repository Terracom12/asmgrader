#!/bin/bash

# TODO: Replace this with CMake targets

# Exit on error
set -e

ROOT_DIR=$(
    cd "$(dirname "${BASH_SOURCE[0]}")"/..
    pwd -P
)

cd "$ROOT_DIR"

failed=0

lint_files=$(
    find src/ include/ cs3b-grader/ tests/ -name '*.hpp' -or -name '*.cpp'
)
compilation_db=$(find build/ -name 'compile_commands.json' | head -n1)

echo
echo "Improper Library Conventions:"
echo "=========================================================="
LIBRARY_NAMES=$(while IFS= read -r line; do
    [[ $line == \#* ]] || break # stop at first non-# line
    echo "${line#\# }"          # strip leading "# "
done <cmake/SetupDependencies.cmake)
LIBRARY_REGEX="\"($(paste -sd'|' <<<"$LIBRARY_NAMES"))/" # replace newlines with '|'s
# Find improper library header conventions ("" instead of <>)
if grep -E "$LIBRARY_REGEX" $lint_files; then
    failed=1
else
    echo "<none>"
fi

echo
echo "Clang Tidy:"
echo "=========================================================="
if ! clang-tidy $lint_files -p "$compilation_db" --export-fixes=reports/clang-tidy.yaml \
    --warnings-as-errors='*' --format-style 'file' --quiet; then
    failed=1
fi

echo
echo "Clang Format:"
echo "=========================================================="
if ! clang-format $lint_files --dry-run -Werror; then
    failed=1
fi

exit $failed
