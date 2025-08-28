#!/bin/bash

set -e

if [[ $1 == "tests" || $1 == "t" ]]; then
    # Tell catch2 to break upon failure, allowing gdb to attach
    EXEC="./build/tests/asmgrader_tests"
    EXEC_ARGS=(--break)
    shift
elif [[ $1 == "prof" || $1 == "p" ]]; then
    EXEC="./build/cs3b-grader/profgrader"
    shift
else
    EXEC="./build/cs3b-grader/grader"
    EXEC_ARGS=()
fi

if [[ $# ]]; then
    GDB_ARGS=("$@")
fi
for d in ./build/_deps/*-src; do
    GDB_ARGS=("--directory" "$d" "${GDB_ARGS[@]}")
done

# logs are probably kind of useless if we know what we're debugging
if [ ! -v LOG_LEVEL ]; then
    LOG_LEVEL=error
fi

# LSAN does not work under gdb
# abortions will cause gdb to break on ASAN errors
gdb --ex 'set environment ASAN_OPTIONS detect_leaks=0:abort_on_error=1' \
    --ex "set environment LOG_LEVEL $LOG_LEVEL" \
    --quiet \
    "${GDB_ARGS[@]}" \
    --args $EXEC "${EXEC_ARGS[@]}"
