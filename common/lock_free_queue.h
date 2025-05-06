#pragma once

#include <iostream>
#include <atomic>
#include <vector>

#include "macros.h"

// single producer single consumer lock free queue

namespace common {

    template<typename T>
    class LockFreeQueue final {
    private:
        std::vector<T> store;
        
        std::atomic<std::size_t> next_read_index {0};
        std::atomic<std::size_t> next_write_index {0};
        std::atomic<std::size_t> num_elements {0};
    
    public: 
        LockFreeQueue(std::size_t elems) : store(elems, T()) {}
    
        LockFreeQueue() = delete;
        LockFreeQueue(const LockFreeQueue&) = delete;
        LockFreeQueue(const LockFreeQueue&&) = delete;
        LockFreeQueue& operator=(const LockFreeQueue&) = delete;
        LockFreeQueue& operator=(const LockFreeQueue&&) = delete;
    
        auto get_next_write() noexcept {
            return &store[next_write_index];
        }
    
        auto update_write_index() noexcept {
            next_write_index = (next_write_index + 1) % store.size();
            ++num_elements;
        }
    
        const T* get_next_read() const noexcept {
            return (size() ? &store[next_read_index] : nullptr);
        }
    
        auto update_read_index() noexcept {
            next_read_index = (next_read_index + 1) % store.size();
            ASSERT(num_elements != 0, "Invalid read operation");
            --num_elements;
        }
    
        auto size() const noexcept {
            return num_elements.load();
        }
    
    };
}


