#!/bin/bash

# Exit if command fails
set -e

if [[ ! -f ./build/tests/asmgrader_tests ]]; then
    echo "Test executable not yet built!"
    exit 1
fi

if [ ! -v LOG_LEVEL ]; then
    export LOG_LEVEL=error
fi

unittests_res=0
if ! ./build/tests/asmgrader_tests; then unittests_res=$?; fi

if [[ -f ./cs3b-grader/test.sh ]]; then
    ./cs3b-grader/test.sh ./build
fi

exit $unittests_res
