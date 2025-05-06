#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "macros.h"

namespace common {

    template <typename T>
    class MemoryPool final {
    public:
        explicit MemoryPool(std::size_t num_elems) : store(num_elems, {T(), true}) {
            ASSERT(reinterpret_cast<const ObjectBlock*>(&(store[0].object)) == &(store[0]),
                "T object should be first member of ObjectBlock.\n");
        }

        // note: most compilers implement placement new with extra if statement to check if memory is non null
        template <typename... Args>
        T* allocate(Args... args) noexcept {
            auto obj_block = &(store[next_free_index]);
            ASSERT(obj_block->is_free, "Expected free ObjectBlock at index:" + std::to_string(next_free_index) + '\n');
            T* ret = &(obj_block->object);
            ret = new(ret) T(args...);
            obj_block->is_free = false;
            
            update_next_free_index();

            return ret;
        }

        auto deallocate(const T* elem) noexcept {
            const auto elem_index = (reinterpret_cast<const ObjectBlock*>(elem) - &store[0]);
            ASSERT(elem_index >= 0 && (static_cast<std::size_t>(elem_index) < store.size()), 
                "Element being deallocated does not belong to this memory pool.\n");
            ASSERT(!store[elem_index].is_free, "Expected in-use ObjectBlock at index:" + std::to_string(elem_index));
            store[elem_index].is_free = true;
        }

        MemoryPool() = delete;
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool(const MemoryPool&&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&&) = delete;

    private:
        struct ObjectBlock {
            T object;
            bool is_free {true};
        };

        std::vector<ObjectBlock> store;
        std::size_t next_free_index {0};

        // we can make this faster, keep simple for now
        auto update_next_free_index() noexcept {
            const auto initial_free_index = next_free_index;
            while (!store[next_free_index].is_free) {
                ++next_free_index;
                if (next_free_index == store.size()) [[unlikely]]
                    next_free_index = 0;
                if (initial_free_index == next_free_index) [[unlikely]]
                    ASSERT(initial_free_index != next_free_index, "Memory pool out of space.\n");
            }
        }
    };




}