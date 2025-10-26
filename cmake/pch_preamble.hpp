#pragma once

#ifndef __clang__

// gcc has been giving some false positives for stdlib headers

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#endif // ! __clang__
