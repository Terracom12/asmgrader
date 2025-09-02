#pragma once

#include <asmgrader/api/asm_buffer.hpp>               // IWYU pragma: export
#include <asmgrader/api/asm_data.hpp>                 // IWYU pragma: export
#include <asmgrader/api/asm_function.hpp>             // IWYU pragma: export
#include <asmgrader/api/asm_symbol.hpp>               // IWYU pragma: export
#include <asmgrader/api/test_base.hpp>                // IWYU pragma: export
#include <asmgrader/api/test_context.hpp>             // IWYU pragma: export
#include <asmgrader/common/aliases.hpp>               // IWYU pragma: export
#include <asmgrader/common/bit_casts.hpp>             // IWYU pragma: export
#include <asmgrader/common/formatters/formatters.hpp> // IWYU pragma: export
#include <asmgrader/logging.hpp>                      // IWYU pragma: export

#include <fmt/format.h> // IWYU pragma: export

// Include just about every stdlib headers that we might possibly want for convenience,
// and to make test sources less visually cluttered
#include <algorithm>   // IWYU pragma: export
#include <array>       // IWYU pragma: export
#include <cerrno>      // IWYU pragma: export
#include <charconv>    // IWYU pragma: export
#include <compare>     // IWYU pragma: export
#include <concepts>    // IWYU pragma: export
#include <csignal>     // IWYU pragma: export
#include <cstddef>     // IWYU pragma: export
#include <cstdio>      // IWYU pragma: export
#include <cstdlib>     // IWYU pragma: export
#include <ctime>       // IWYU pragma: export
#include <filesystem>  // IWYU pragma: export
#include <functional>  // IWYU pragma: export
#include <iostream>    // IWYU pragma: export
#include <limits>      // IWYU pragma: export
#include <memory>      // IWYU pragma: export
#include <numeric>     // IWYU pragma: export
#include <optional>    // IWYU pragma: export
#include <regex>       // IWYU pragma: export
#include <string>      // IWYU pragma: export
#include <string_view> // IWYU pragma: export
#include <tuple>       // IWYU pragma: export
#include <type_traits> // IWYU pragma: export
#include <utility>     // IWYU pragma: export
#include <variant>     // IWYU pragma: export
#include <vector>      // IWYU pragma: export

// should always include last if possible, as the short macro names may conflict with other libraries.
#include <asmgrader/api/test_macros.hpp> // IWYU pragma: export
