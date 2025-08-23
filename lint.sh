#!/bin/bash

# Exit on error
set -e

ROOT_DIR=$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd -P
)

cd "$ROOT_DIR"

failed=0

lint_files=$(
    find core/ cs3b-grader/ tests/ -name '*.hpp' -or -name '*.cpp'
)
compilation_db=build/compile_commands.json

cmake_files=$(
    find . -name 'CMakeLists.txt' -not -path 'build'
)

echo "Missing CMake Includes:"
echo "=========================================================="
for f in $lint_files; do
    stripped_path="${f#*/}" # remove top-level dir
    if ! grep -Fq "$stripped_path" $cmake_files; then
        echo "$f"
        failed=1
    fi
done
if [[ $failed -eq 0 ]]; then
    echo "<none>"
fi

echo
echo "Improper Library Conventions:"
echo "=========================================================="
# Find improper library header conventions ("" instead of <>)
if grep -E '"(range|boost|fmt|catch2|nlohmann|argparse|gsl)/' $lint_files; then
    failed=1
else
    echo "<none>"
fi

echo
echo "Clang Tidy:"
echo "=========================================================="
if ! clang-tidy $lint_files -p "$compilation_db" --warnings-as-errors='*'; then
    failed=1
fi

echo
echo "Clang Format:"
echo "=========================================================="
if ! clang-format $lint_files --dry-run -Werror; then
    failed=1
fi

exit $failed
