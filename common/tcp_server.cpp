#include "tcp_server.h"

namespace Common {
    auto TCPServer::add_to_epoll_list(TCPSocket* socket) {
        epoll_event ev{EPOLLET | EPOLLIN, {reinterpret_cast<void*>(socket)}};
        return !epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->socket_fd_, &ev);
    }



}