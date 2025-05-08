#pragma once

#include <functional>

#include "socket_utils.h"
#include "logging.h"

namespace Common {

    constexpr std::size_t TCP_BUFFER_SIZE = 64 * 1024 * 1024;
    
    struct TCPSocket {
        int socket_fd_ = -1;

        std::vector<char> outbound_data_;
        std::size_t next_send_valid_index_ = 0;
        std::vector<char> inbound_data_;
        std::size_t next_recv_valid_index_ = 0;

        struct sockaddr_in socket_attrib_{};
        
        std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger& logger_;

        explicit TCPSocket(Logger& logger) : logger_{logger} {
            outbound_data_.resize(TCP_BUFFER_SIZE);
            inbound_data_.resize(TCP_BUFFER_SIZE);
        }

        TCPSocket() = delete;
        TCPSocket(const TCPSocket&) = delete;
        TCPSocket& operator=(const TCPSocket&) = delete;
        TCPSocket(TCPSocket&&) = delete;
        TCPSocket& operator=(TCPSocket&&) = delete;

        int connect(const std::string& ip, const std::string& iface, int port, bool is_listening);
        bool send_and_recv() noexcept;
        void send(const void* data, std::size_t len) noexcept;
    };
}