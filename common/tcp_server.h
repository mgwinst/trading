#pragma once

#include "tcp_socket.h"

namespace Common {

    struct TCPServer {
        int epoll_fd_ = -1;       
        TCPSocket listener_socket_;
        epoll_event events_[1024];

        // collection of all sockets, sockets for incoming data, outgoing data and dead connections
        std::vector<TCPSocket*> receive_sockets_, send_sockets_;

        // function wrapper to call back when data is available
        std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback_ = nullptr;
        
        // function wrapper to call back when all data across all TCPSockets have been read and dispatched this round
        std::function<void()> recv_finished_callback_ = nullptr;

        std::string time_str_;
        Logger& logger_;

        explicit TCPServer(Logger& logger) : listener_socket_{logger}, logger_{logger} {}

        TCPServer() = delete;
        TCPServer(const TCPSocket &) = delete;
        TCPServer &operator=(const TCPSocket &) = delete;
        TCPServer(TCPSocket &&) = delete;
        TCPServer &operator=(TCPSocket &&) = delete;

        void listen(const std::string& iface, int port);
        void poll() noexcept;
        void send_and_recv() noexcept;

        private:
            bool add_to_epoll_list(TCPSocket* socket);

    };



}
