#pragma once

#include <string>
#include <fstream>
#include <chrono>

#include "macros.h"
#include "thread_utils.h"
#include "lock_free_queue.h"
#include "time_utils.h"

namespace common {

    constexpr std::size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

    enum class LogType : int8_t {
        CHAR = 0,
        INT = 1, 
        LONG_INT = 2,
        LONG_LONG_INT = 3,
        UNSIGNED_INT = 4,
        UNSIGNED_LONG_INT = 5,
        UNSIGNED_LONG_LONG_INT = 6,
        FLOAT = 7,
        DOUBLE = 8
    };

    struct LogElement {
        LogType type = LogType::CHAR;
        union {
            char c;
            int i;
            long l;
            long long ll;
            unsigned u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;
        } prims;
    };

    class Logger final {
    private:
        const std::string log_file_name;
        std::ofstream log_file;
        LockFreeQueue<LogElement> log_queue;
        std::atomic_bool running = {true};
        std::thread* logger_thread = nullptr;

    public:

        auto flush_queue() noexcept {
            while (running) {
                for (auto next = log_queue.get_next_read(); log_queue.size() && next; next = log_queue.get_next_read()) {
                    switch (next->type) {
                        case LogType::CHAR:
                            log_file << next->prims.c;
                            break;
                        case LogType::INT:
                            log_file << next->prims.i;
                            break;
                        case LogType::LONG_INT:
                            log_file << next->prims.l;
                            break;
                        case LogType::LONG_LONG_INT:
                            log_file << next->prims.ll;
                            break;
                        case LogType::UNSIGNED_INT:
                            log_file << next->prims.u;
                            break;
                        case LogType::UNSIGNED_LONG_INT:
                            log_file << next->prims.ul;
                            break;
                        case LogType::UNSIGNED_LONG_LONG_INT:
                            log_file << next->prims.ull;
                            break;
                        case LogType::FLOAT:
                            log_file << next->prims.f;
                            break;
                        case LogType::DOUBLE:
                            log_file << next->prims.d;
                            break;
                    }
                    log_queue.update_read_index();
                }
                log_file.flush(); // flush rest of buffer to file
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        explicit Logger(const std::string& file_name) : log_file_name{file_name}, log_queue{LOG_QUEUE_SIZE} {
            log_file.open(file_name);
            ASSERT(log_file.is_open(), "Could not open log file: " + file_name + "\n");
            logger_thread = create_and_start_thread(-1, "Common/Logger", [this]() { flush_queue(); });
            ASSERT(logger_thread != nullptr, "Failed to start logger thread\n");
        }
        
        ~Logger() {
            std::string time_str;
            std::cerr << common::get_current_time_str(time_str) << " Flushing and closing Logger for " << log_file_name << '\n';

            while (log_queue.size()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            running = false;
            logger_thread->join();

            log_file.close();
            std::cerr << common::get_current_time_str(time_str) << " Logger for " << log_file_name << " exiting.\n";
        }

        Logger() = delete;
        Logger(const Logger&) = delete;
        Logger(const Logger&&) = delete;
        Logger& operator=(const Logger &) = delete;
        Logger& operator=(const Logger &&) = delete;
        
        auto push_value(const LogElement& log_elem) noexcept {
            *(log_queue.get_next_write()) = log_elem;
            log_queue.update_write_index();
        }

        auto push_value(const char value) noexcept {
            push_value(LogElement{LogType::CHAR, {.c = value}});
        }
      
        auto push_value(const int value) noexcept {
            push_value(LogElement{LogType::INT, {.i = value}});
        }
      
        auto push_value(const long value) noexcept {
            push_value(LogElement{LogType::LONG_INT, {.l = value}});
        }
      
        auto push_value(const long long value) noexcept {
            push_value(LogElement{LogType::LONG_LONG_INT, {.ll = value}});
        }
      
        auto push_value(const unsigned value) noexcept {
            push_value(LogElement{LogType::UNSIGNED_INT, {.u = value}});
        }
      
        auto push_value(const unsigned long value) noexcept {
            push_value(LogElement{LogType::UNSIGNED_LONG_INT, {.ul = value}});
        }
      
        auto push_value(const unsigned long long value) noexcept {
            push_value(LogElement{LogType::UNSIGNED_LONG_LONG_INT, {.ull = value}});
        }
      
        auto push_value(const float value) noexcept {
            push_value(LogElement{LogType::FLOAT, {.f = value}});
        }
      
        auto push_value(const double value) noexcept {
            push_value(LogElement{LogType::DOUBLE, {.d = value}});
        }

        auto push_value(const char* value) noexcept {
            while (*value) {
                push_value(*value);
                value++;
            }
        }

        auto push_value(const std::string& value) noexcept {
            push_value(value.c_str());
        }

        template <typename T, typename... Args>
        auto log(const char* s, const T& value, Args... args) noexcept {
            while(*s) {
                if (*s == '%') {
                    push_value(value);
                    log(s + 1, args...);
                    return;
                }
                push_value(*s++);
            }
        }

        auto log(const char* s) noexcept {
            while (*s) {
                if (*s == '%') {
                    FATAL("Missing args to log()");
                }
                push_value(*s++);
            }
        }
    };
}



