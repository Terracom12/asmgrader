# !!DISCLAIMER!!
# This was adapted from the following source
#   jeremy-rifkin/cpptrace
#   https://github.com/jeremy-rifkin/cpptrace/tree/b724d1328ced0d71315c28f6ec6e11d0652062bc
#   Courtesy of jeremy-rifkin
# Who themselves adapted their's from the following source
#   compiler-explorer/compiler-explorer
#   https://github.com/compiler-explorer/compiler-explorer/blob/main/Makefile
#   Courtesy of Matt Godbolt, et al.

# https://gist.github.com/bbl/5429c4a0a498c5ef91c10201e1b651c2

# Intended user-args
CTEST_EXTRA_ARGS ?=
CMAKE_CONFIGURE_EXTRA_ARGS ?=
CMAKE_BUILD_EXTRA_ARGS ?=
# gcc or clang
COMPILER ?= gcc
NUM_JOBS ?= $(shell echo $$(( $$(nproc) / 2 )))

ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

BUILD_DIR := $(ROOT_DIR)/build
SOURCE_DIR := $(ROOT_DIR)

DEBUG_PRESET := unixlike-$(COMPILER)-debug
RELEASE_PRESET := unixlike-$(COMPILER)-release
DOCS_PRESET := unixlike-docs-only

SRC_ENV := if [ -f "$(ROOT_DIR)/.env" ]; then export $$(cat "$(ROOT_DIR)/.env" | xargs); echo "Set enviornment variables:"; sed -E 's/=.*//' "$(ROOT_DIR)/.env"; echo; fi

NUM_JOBS := $(shell echo $$(( $$(nproc) / 2 )))

default: help

##### --- snipped from gh:compiler-explorer/compiler-explorer/blob/main/Makefile
help: # with thanks to Ben Rady
	@printf '\033[33mOPTIONS:\033[0m\n'
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'
#### --- END snip

# Each preset will create a subdirectory within build
# CMake is smart when using --preset --build and will automatically re-configure
# if any cmake sources change, so we don't have to handle that manually here.

$(BUILD_DIR)/$(DEBUG_PRESET):
	@$(SRC_ENV) && cmake --preset $(DEBUG_PRESET) $(CMAKE_CONFIGURE_EXTRA_ARGS) -j $(NUM_JOBS)

$(BUILD_DIR)/$(RELEASE_PRESET):
	@$(SRC_ENV) && cmake --preset $(RELEASE_PRESET) $(CMAKE_CONFIGURE_EXTRA_ARGS) -j $(NUM_JOBS)

$(BUILD_DIR)/$(DOCS_PRESET):
	@$(SRC_ENV) && cmake --preset $(DOCS_PRESET) $(CMAKE_CONFIGURE_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: configure-debug
configure-debug: $(BUILD_DIR)/$(DEBUG_PRESET)

.PHONY: configure-release
configure-release: $(BUILD_DIR)/$(RELEASE_PRESET)

.PHONY: configure-docs
configure-docs: $(BUILD_DIR)/$(DOCS_PRESET)


.PHONY: build
build: build-debug  ## alias for build-debug

.PHONY: build-debug
build-debug: configure-debug ## build in debug mode
	@$(SRC_ENV) && cmake --build --preset $(DEBUG_PRESET) $(CMAKE_BUILD_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: build-release
build-release: configure-release ## build in release mode (with debug info)
	@$(SRC_ENV) && cmake --build --preset $(RELEASE_PRESET) $(CMAKE_BUILD_EXTRA_ARGS) -j $(NUM_JOBS)
	
.PHONY: build-docs
build-docs: configure-docs ## build in release mode (with debug info)
	@$(SRC_ENV) && cmake --build --preset $(DOCS_PRESET) $(CMAKE_BUILD_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: build-tests-debug
build-tests-debug: configure-debug
	@$(SRC_ENV) && cmake --build --preset $(DEBUG_PRESET) --target asmgrader_tests $(CMAKE_BUILD_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: build-tests-release
build-tests-release: configure-release
	@$(SRC_ENV) && cmake --build --preset $(RELEASE_PRESET) --target asmgrader_tests $(CMAKE_BUILD_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: test
test: test-debug  ## alias for test-debug

.PHONY: test-debug
test-debug: build-tests-debug ## run tests in debug mode
	 @$(SRC_ENV) && ctest --preset $(DEBUG_PRESET) --progress --output-on-failure --no-tests=error $(CTEST_EXTRA_ARGS) -j $(NUM_JOBS)

.PHONY: test-release
test-release: build-tests-release ## run tests in release mode
	 @$(SRC_ENV) && ctest --preset $(RELEASE_PRESET) --progress --output-on-failure --no-tests=error $(CTEST_EXTRA_ARGS) -j $(NUM_JOBS)


.PHONY: clean
clean: ## remove build objects, libraries, executables, and test reports
	rm -rf reports/*
	-cmake --build --preset $(DEBUG_PRESET) --target clean
	-cmake --build --preset $(RELEASE_PRESET) --target clean
	-cmake --build --preset $(DOCS_PRESET) --target clean

.PHONY: deep-clean
deep-clean: ## remove all build files and configuration
	rm -rf $(BUILD_DIR)/ reports/*

.PHONY: list-opts
list-opts: ## list available CMake options
	@if ! cmake -S . -B $(shell find build/ -maxdepth 1 -mindepth 1 | head -n1) -LAH -N 2>/dev/null \
			| grep ASMGRADER_ --before-context 1 --group-separator='' \
			| sed '/\/\// { s/^/[34m/; s/$$/[0m/; }' \
			| sed '/^\w/  { s/^/[32m/; s/:/[0m:/; }' \
			; then \
		printf '\033[31mError: CMake command failed. Is the project configured?\033[0m\n'; \
		exit 1; \
	fi
