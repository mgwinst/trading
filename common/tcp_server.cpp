#include "tcp_server.h"

namespace Common {

    bool TCPServer::add_to_epoll_list(TCPSocket* socket) {
        epoll_event ev{EPOLLET | EPOLLIN, {reinterpret_cast<void*>(socket)}};
        return !epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->socket_fd_, &ev);
    }

    void TCPServer::listen(const std::string& iface, int port) {
        epoll_fd_ = epoll_create(1);
        ASSERT(epoll_fd_ >= 0, "epoll_create() failed. error: " + std::string{strerror(errno)});
        ASSERT(listener_socket_.connect("", iface, port, true) >= 0, "TCPSocket::connect() failed (listener socket). iface: " + iface + "port: " + std::to_string(port) + "error: " + std::string{strerror(errno)});
        ASSERT(add_to_epoll_list(&listener_socket_), "epoll_ctl() failed. errno: " + std::string{strerror(errno)});
    }

    void TCPServer::send_and_recv() noexcept {
        auto recv = false;
        std::for_each(std::begin(receive_sockets_), std::end(receive_sockets_), [&recv](auto socket) {
            recv |= socket->send_and_recv();
        });

        // there were some events and they have all been dispatched, inform listener
        if (recv && recv_callback_) recv_finished_callback_();

        std::for_each(std::begin(send_sockets_), std::end(send_sockets_), [](auto socket) {
            socket->send_and_recv();
        });
    }

    // check for new connections or dead connections and update containers that track the sockets
    void TCPServer::poll() noexcept {
        const int max_events = 1 + send_sockets_.size() + receive_sockets_.size();
        const int n = epoll_wait(epoll_fd_, events_, max_events, 0);
        bool have_new_connection = false;
        for (auto i = 0; i < n; i++) {
            const auto &event = events_[i];
            auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);

            // Check for new connections.
            if (event.events & EPOLLIN) {
                if (socket == &listener_socket_) {
                    logger_.log("%:% %() % EPOLLIN listener_socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                        Common::get_current_time_str(time_str_), socket->socket_fd_);
                    have_new_connection = true;
                    continue;
                }
                logger_.log("%:% %() % EPOLLIN socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::get_current_time_str(time_str_), socket->socket_fd_);
                if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                    receive_sockets_.push_back(socket);
            }

            if (event.events & EPOLLOUT) {
                logger_.log("%:% %() % EPOLLOUT socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::get_current_time_str(time_str_), socket->socket_fd_);
                if (std::find(send_sockets_.begin(), send_sockets_.end(), socket) == send_sockets_.end())
                    send_sockets_.push_back(socket);
            }

            if (event.events & (EPOLLERR | EPOLLHUP)) {
                logger_.log("%:% %() % EPOLLERR socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::get_current_time_str(time_str_), socket->socket_fd_);
                if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                    receive_sockets_.push_back(socket);
            }
        }

        // Accept a new connection, create a TCPSocket and add it to our containers.
        while (have_new_connection) {
            logger_.log("%:% %() % have_new_connection\n", __FILE__, __LINE__, __FUNCTION__,
                Common::get_current_time_str(time_str_));
            sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            int fd = accept(listener_socket_.socket_fd_, reinterpret_cast<sockaddr *>(&addr), &addr_len);
            if (fd == -1)
                break;

            ASSERT(set_non_blocking(fd) && disable_naggle(fd),
                   "Failed to set non-blocking or no-delay on socket:" + std::to_string(fd));

            logger_.log("%:% %() % accepted socket:%\n", __FILE__, __LINE__, __FUNCTION__,
                        Common::get_current_time_str(time_str_), fd);

            auto socket = new TCPSocket(logger_);
            socket->socket_fd_ = fd;
            socket->recv_callback_ = recv_callback_;
            ASSERT(add_to_epoll_list(socket), "Unable to add socket. error:" + std::string(std::strerror(errno)));

            if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                receive_sockets_.push_back(socket);
        }
    }
}



