#!/bin/bash

# Exit if command fails
set -e

./build.sh

./build/test/tests
