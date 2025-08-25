#!/bin/bash

# Exit if command fails
set -e

if [[ -d ./build && -z $(find ./build -name CTestTestfile.cmake) ]]; then
    echo "Tests not yet built!"
    exit 1
fi

TEST_CONFIG=
if [[ $# -gt 0 ]]; then
    TEST_CONFIG="$1"
fi

cd build/
ctest -C "$TEST_CONFIG"
