#!/bin/bash

# Exit upon command failure
set -e

SCRIPT_DIR=$(
    cd "$(dirname "${BASH_SOURCE[0]}")"
    pwd -P
)
cd "$SCRIPT_DIR"

PRE_BUILD_ARGS=("-DENABLE_TESTING:BOOL:=TRUE" "-DCMAKE_BUILD_TYPE:STRING:=Debug" "-DFETCHCONTENT_UPDATES_DISCONNECTED:BOOL:=TRUE")
BUILD_ARGS=()

BUILD_DIR="./build"

if [[ "$1" == "aarch64" ]]; then
    echo "Attempting to cross-compile for aarch64"

    BUILD_DIR="$BUILD_DIR"/aarch64
    PRE_BUILD_ARGS=("${PRE_BUILD_ARGS[@]}" "-DCMAKE_TOOLCHAIN_FILE:=./cmake/aarch64-toolchain.cmake")
    shift
fi

if [[ $# -gt 0 ]]; then
    BUILD_ARGS=("${BUILD_ARGS[@]}" "$@")
fi

cmake -B $BUILD_DIR -S . "${PRE_BUILD_ARGS[@]}"
cmake --build $BUILD_DIR "${BUILD_ARGS[@]}"
