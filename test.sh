#!/bin/bash

# Exit if command fails
set -e

./build.sh

if [ ! -v LOG_LEVEL ]; then
    export LOG_LEVEL=error
fi

unittests_res=0
if ! ./build/tests/asmgrader_tests; then unittests_res=$?; fi

if [[ -f ./grader/run-tests.sh ]]; then
    ./grader/run-tests.sh ./build
fi

exit $unittests_res
