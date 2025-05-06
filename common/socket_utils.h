#pragma once

#include <iostream>
#include <string>
#include <unordered_set>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "macros.h"
#include "logging.h"

namespace common {

    constexpr int max_tcp_server_backlog = 1024;

    std::string get_iface_ip(const std::string& iface);
    bool set_non_blocking(int fd);
    bool set_no_delay(int fd);
    bool set_so_time_stamp(int fd);
    bool would_block();
    bool set_mcast_ttl(int fd, int ttl);
    bool set_ttl(int fd, int ttl);
    bool join(int fd, const std::string& ip, const std::string& iface, int port);
    int create_socket(Logger& logger, const std::string& t_ip, const std::string& iface,
        int port, bool is_udp, bool is_blocking, bool is_listening, int ttl, bool needs_so_timestamp);

}