#pragma once

#include <asmgrader/subprocess/memory/memory_io_base.hpp>   // IWYU pragma: export
#include <asmgrader/subprocess/memory/memory_io_serde.hpp>  // IWYU pragma: export
#include <asmgrader/subprocess/memory/ptrace_memory_io.hpp> // IWYU pragma: export

// TODO: If efficiency with ptrace becomes an issue, consider an implementation with mmap
