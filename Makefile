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
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

BUILD_DIR := $(ROOT_DIR)/build
SOURCE_DIR := $(ROOT_DIR)

CMAKE_SOURCES := $(shell find $(SOURCE_DIR) -path $(BUILD_DIR) -prune -a -name 'CMakeLists.txt' -o -name '*.cmake') src/version.hpp.in CMakePresets.json
CMAKE_SOURCES_HASH := $(shell cat $(CMAKE_SOURCES) | sha256sum | awk '{ print $$1 }')
CMAKE_HASH_CACHE_FILE := $(BUILD_DIR)/cmake-sources-hash.txt

DEBUG_PRESET := unixlike-gcc-debug
RELEASE_PRESET := unixlike-gcc-release

SRC_ENV := if [ -f "$(ROOT_DIR)/.env" ]; then export $$(cat "$(ROOT_DIR)/.env" | xargs); echo "Set enviornment variables:"; sed -E 's/=.*//' "$(ROOT_DIR)/.env"; echo; fi

default: help

##### --- snipped from gh:compiler-explorer/compiler-explorer/blob/main/Makefile
help: # with thanks to Ben Rady
	@printf '\033[33mOPTIONS:\033[0m\n'
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'
#### --- END snip

# Each preset will create a subdirectory within build

$(BUILD_DIR)/$(DEBUG_PRESET):
	@$(SRC_ENV) && cmake --preset $(DEBUG_PRESET)

$(BUILD_DIR)/$(RELEASE_PRESET):
	@$(SRC_ENV) && cmake --preset $(RELEASE_PRESET)

.PHONY: configure-debug
configure-debug: $(BUILD_DIR)/$(DEBUG_PRESET)

.PHONY: configure-release
configure-release: $(BUILD_DIR)/$(RELEASE_PRESET)

.PHONY: build
build: build-debug  ## alias for build-debug

.PHONY: build-debug
build-debug: configure-debug ## build in debug mode
	@$(SRC_ENV) && cmake --build --preset $(DEBUG_PRESET)

.PHONY: build-release
build-release: configure-release ## build in release mode (with debug info)
	@$(SRC_ENV) && cmake --build --preset $(RELEASE_PRESET)

.PHONY: test
test: test-debug  ## alias for test-debug

.PHONY: test-debug
test-debug: build-debug ## run tests in debug mode
	 @$(SRC_ENV) && ctest --test-dir $(BUILD_DIR) --progress --output-on-failure

.PHONY: test-release
test-release: build-release  ## run tests in release mode
	 @$(SRC_ENV) && ctest --preset $() --progress --output-on-failure


.PHONY: clean
clean: ## remove build objects, libraries, executables, and test reports
	rm -rf reports/*
	cmake --build --preset $(DEBUG_PRESET) --target clean
	cmake --build --preset $(RELEASE_PRESET) --target clean

.PHONY: deep-clean
deep-clean: ## remove all build files and configuration
	rm -rf $(BUILD_DIR)/ reports/*

# .PHONY: reconfigure-warning
# reconfigure-warning:
# 	@if [ -z "build/configured-*" ]; then \
# 		true; \
# 	elif [ ! -f $(CMAKE_HASH_CACHE_FILE) ]; then \
# 		printf '\033[33mWarning: CMake sources have not been hashed; You may want to clean and reconfigure.\nUse `make deep-clean` and `make configure-<mode>`.\033[0m\n'; \
# 	elif [ "$$(cat $(CMAKE_HASH_CACHE_FILE))" != "$(CMAKE_SOURCES_HASH)" ]; then \
# 		printf '\033[33mWarning: CMake sources hash has changed; You may want to clean and reconfigure.\nUse `make deep-clean` and `make configure-<mode>`.\033[0m\n'; \
# 	fi
# 	@mkdir -p $$(dirname $(CMAKE_HASH_CACHE_FILE))
# 	@echo $(CMAKE_SOURCES_HASH) > $(CMAKE_HASH_CACHE_FILE)

.PHONY: list-opts
list-opts: ## list available CMake options
	@if ! cmake -S . -B build/ -LAH -N 2>/dev/null \
			| grep ASMGRADER_ -B 1 \
			; then \
		printf '\033[31mError: CMake command failed. Is the project configured?\033[0m\n'; \
		exit 1; \
	fi
