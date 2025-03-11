#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

// list of common macros that might be useful

// branch prediction compiler opts

#define LIKELY(x)  __builtin_expect(!!(x), 1)
#define UNLIKELY(x)  __builtin_expect(!!(x), 0)

// custom assertions with error messages

inline auto ASSERT(bool cond, const std::string& msg) noexcept {
    if (UNLIKELY(!cond)) {
        std::cerr << msg << '\n';
        exit(EXIT_FAILURE);
    }
}

inline auto FATAL(const std::string& msg) noexcept {
    std::cerr << msg << '\n';
    exit(EXIT_FAILURE);
}



