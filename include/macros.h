#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

// biased branch macros

#define LIKELY(x)  __builtin_expect(!!(x), 1)
#define UNLIKELY(x)  __builtin_expect(!!(x), 0)

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

