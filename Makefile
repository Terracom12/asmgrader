# !!DISCLAIMER!!
# This was adapted from the following source
#   jeremy-rifkin/cpptrace
#   https://github.com/jeremy-rifkin/cpptrace/tree/b724d1328ced0d71315c28f6ec6e11d0652062bc
#   Courtesy of jeremy-rifkin
# Who themselves adapted their's from the following source
#   compiler-explorer/compiler-explorer
#   https://github.com/compiler-explorer/compiler-explorer/blob/main/Makefile
#   Courtesy of Matt Godbolt, et al.

CONFIGURE_OPTS := -DASMGRADER_BUILD_TESTS=ON -DASMGRADER_ENABLE_CACHE=ON -DASMGRADER_ENABLE_PCH=ON
GENERATOR := Ninja

# https://gist.github.com/bbl/5429c4a0a498c5ef91c10201e1b651c2
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build
SOURCE_DIR := $(ROOT_DIR)

default: help

##### --- snipped from gh:compiler-explorer/compiler-explorer/blob/main/Makefile
help: # with thanks to Ben Rady
	@printf '\033[33mOPTIONS:\033[0m\n'
	@grep -E '^[0-9a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'
#### --- END snip

.PHONY: build
build: debug  ## build in debug mode

$(BUILD_DIR)/configured-debug:
	cmake -S $(SOURCE_DIR) -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=Debug $(CONFIGURE_OPTS)
	rm -f $(BUILD_DIR)/configured-release
	touch $(BUILD_DIR)/configured-debug

$(BUILD_DIR)/configured-release:
	cmake -S $(SOURCE_DIR) -B $(BUILD_DIR) -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo $(CONFIGURE_OPTS)
	rm -f $(BUILD_DIR)/configured-debug
	touch $(BUILD_DIR)/configured-release

.PHONY: configure-debug
configure-debug: $(BUILD_DIR)/configured-debug

.PHONY: configure-release
configure-release: $(BUILD_DIR)/configured-release

.PHONY: debug
debug: configure-debug  ## build in debug mode
	cmake --build $(BUILD_DIR)

.PHONY: release
release: configure-release  ## build in release mode (with debug info)
	cmake --build $(BUILD_DIR)

.PHONY: clean
clean: ## remove build objects, libraries, and executables
	rm -f $(BUILD_DIR)/configured-debug $(BUILD_DIR)/configured-release
	cmake --build $(BUILD_DIR) --target clean

.PHONY: deep-clean
deep-clean: ## remove all build files and configuration
	rm -rf $(BUILD_DIR)/

.PHONY: test
test: debug  ## test
	 ctest --test-dir $(BUILD_DIR) --progress --output-on-failure

.PHONY: test-release
test-release: release  ## test-release
	 ctest --test-dir $(BUILD_DIR) --progress --output-on-failure

.PHONY: docs
docs:  configure-release ## build project documentation (Doxygen)
	cmake --build build --target doxygen-docs

.PHONY: list-opts
list-opts: ## list available CMake options
	@if ! cmake -S . -B build/ -LAH -N 2>/dev/null \
			| grep ASMGRADER_ -B 1 \
			; then \
		printf '\033[31mError: CMake command failed. Is the project configured?\033[0m\n'; \
		exit 1; \
	fi
