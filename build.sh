#!/bin/bash

BUILD_ARGS=()
if [[ $# -gt 0 ]]; then
    BUILD_ARGS=("${BUILD_ARGS[@]}" "$@")
fi

cmake -B build -S . -DENABLE_TESTING:BOOL:=TRUE -DCMAKE_BUILD_TYPE:STRING:=Debug -DFETCHCONTENT_UPDATES_DISCONNECTED:BOOL:=TRUE
cmake --build build "${BUILD_ARGS[@]}"
