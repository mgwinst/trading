#pragma once

#include <chrono>
#include <ctime>

namespace common {
    using Nanos = int64_t;
    
    inline auto getCurrentNanos() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    inline auto& get_current_time_str(std::string& time_str) {
        const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        time_str.append(ctime(&time));
        if (!time_str.empty())
            time_str.at(time_str.length()-1) = '\0';
        return time_str;
    }
}