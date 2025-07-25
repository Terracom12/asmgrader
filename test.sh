#!/bin/bash

# Exit if command fails
set -e

./build.sh

if [ ! -v LOG_LEVEL ]; then
    export LOG_LEVEL=error
fi

./build/test/tests
