#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>


namespace Common {

    inline auto set_thread_core(int core_id) noexcept {
        cpu_set_t cpuset; // cpuset bitmask
        CPU_ZERO(&cpuset); // clear the set
        CPU_SET(core_id, &cpuset); // add core to the set
        return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0);   
    }

    template <typename F, typename... Args>
    inline auto create_and_start_thread(int core_id, const std::string& name, F&& func, Args&&... args) noexcept {
        std::atomic_bool running {0};
        std::atomic_bool failed {0};

        auto thread_body = [&] {
            if (core_id >= 0 && !set_thread_core(core_id)) {
                std::cerr << "Failed to set core affinity for " << name << ' ' << 
                pthread_self() << " to " << core_id << '\n';
                failed = true;
                return;
            }
            std::cout << "Set core affinity for " << name << ' ' << pthread_self() << " to " << core_id << '\n';
            running = true;
            std::forward<F>(func) ((std::forward<Args>(args))...);
        };

        auto t = new std::thread {thread_body};
        
        // wait for new thread's status, either running = true or failed = true, only then should we decide what to do
        while (!running && !failed) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (failed) {
            t->join();
            delete t;
            t = nullptr;
        }

        return t;
    }

}

