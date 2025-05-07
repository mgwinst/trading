#include <iostream>
#include <string>
#include <unordered_set>
#include <sstream>
#include <cstring>
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

#include "logging.h"
#include "macros.h"

namespace Common {

    constexpr int max_tcp_server_backlog = 1024; // max number of pending TCP connections
    
    struct SocketCfg {
        std::string ip {};
        std::string iface {};
        int port = -1;
        bool is_udp = false;
        bool is_listening = false;
        bool needs_so_timestamp = false;

        // replace this using modern std::print, works for now
        auto to_string() const {
            std::stringstream ss;
            ss << "SocketCfg[ip: " << ip
            << " iface: " << iface
            << " port: " << port
            << " is_udp: " << is_udp
            << " is_listening: " << is_listening
            << " needs_so_timestamp: " << needs_so_timestamp << ']';
            
            return ss.str();
        }
    };

    // convert interface name to ip address
    inline std::string get_iface_ip(const std::string& iface) {
        char buf[NI_MAXHOST] = {'\0'};
        ifaddrs* ifaddr = nullptr;
        
        if (getifaddrs(&ifaddr) != -1) {
            ifaddrs* ifa = ifaddr;
            while (ifa) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name) {
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
                    break;
                }
                ifa = ifa->ifa_next;
            }
        }

        freeifaddrs(ifaddr);
        return std::string{buf};
    }

    // set socket to non blocking using fcntl system call (we use fcntl to control file descriptor behavior)
    inline bool set_non_blocking(int fd) {
        const auto flags = fcntl(fd, F_GETFL, 0); // this returns the flags associated with fd currently
        if (flags == -1) return false;
        if (flags & O_NONBLOCK) return true;
        return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
    }

    // disabling Nagle's algorithm
    inline bool disable_naggle(int fd) {
        int one = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void*>(&one), sizeof(one)) == 0);
    }

    // software timestamping on incoming packets
    inline bool set_so_time_stamp(int fd) {
        int one = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, reinterpret_cast<void*>(&one), sizeof(one)) == 0);
    }

    inline bool would_block() {
        return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    }

    inline bool set_TTL(int fd, int ttl) {
        return (setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast<void*>(&ttl), sizeof(ttl)) == 0);
    }

    inline bool set_mcast_TTL(int fd, int mcast_ttl) {
        return (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<void*>(&mcast_ttl), sizeof(mcast_ttl)) == 0);
    }

    [[nodiscard]] inline int create_socket(Logger& logger, const SocketCfg& socket_cfg) {
        std::string time_str{};
        const auto ip = socket_cfg.ip.empty() ? get_iface_ip(socket_cfg.iface) : socket_cfg.ip;

        logger.log("%:% %() % cfg:%\n", __FILE__, __LINE__, __FUNCTION__,
            Common::get_current_time_str(time_str), socket_cfg.to_string());
        
        addrinfo hints{}; // C++ standard ensures POD type will zero out members, don't need to call memset(&hints, 0, sizeof(hints))
        hints.ai_family = AF_INET;
        hints.ai_socktype = socket_cfg.is_udp ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_protocol = socket_cfg.is_udp ? IPPROTO_UDP : IPPROTO_TCP;
        hints.ai_flags = (socket_cfg.is_listening ? AI_PASSIVE : 0) | AI_NUMERICHOST | AI_NUMERICSERV;
        
        addrinfo *result = nullptr;
        const auto retval = getaddrinfo(socket_cfg.ip.c_str(), std::to_string(socket_cfg.port).c_str(), &hints, &result);
        ASSERT(!retval, "getaddrinfo() failed. error: " + std::string{gai_strerror(retval)} + "errno: " + std::string{strerror(errno)});
        
        int socket_fd = -1;
        int one = 1;
        
        addrinfo* rp = result;
        while (rp) {
            ASSERT((socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1, "socket() failed. errno: " + std::string{strerror(errno)});
            ASSERT(set_non_blocking(socket_fd), "set_non_blocking() failed. errno: " + std::string{strerror(errno)});
            if (!socket_cfg.is_udp)
                ASSERT(disable_naggle(socket_fd), "disable_naggle() failed. errno: " + std::string{strerror(errno)});
            if (socket_cfg.is_listening) {
                ASSERT(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<void*>(&one), sizeof(one)) == 0, "setsockopt() SO_REUSEADDR failed. errno: " + std::string{strerror(errno)});
                const sockaddr_in addr{AF_INET, htons(socket_cfg.port), htonl(INADDR_ANY), {}};
                ASSERT(bind(socket_fd, socket_cfg.is_udp ? reinterpret_cast<const sockaddr*>(&addr) : rp->ai_addr, sizeof(addr)) == 0, "bind() failed. errno:%" + std::string(strerror(errno))); 
            }
            if (!socket_cfg.is_udp && socket_cfg.is_listening) // listen for incoming TCP connections
                ASSERT(listen(socket_fd, max_tcp_server_backlog) == 0, "listen() failed, errno: " + std::string{strerror(errno)});
            if (!socket_cfg.is_listening)
                ASSERT(connect(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0, "connect() failed. errno: " + std::string{strerror(errno)});
            if (socket_cfg.needs_so_timestamp)
                ASSERT(set_so_time_stamp(socket_fd), "set_so_timestamp() failed. errno: " + std::string{strerror(errno)});
        }

        freeaddrinfo(result);
        return socket_fd;
    }
}

int main() {
    auto ip_address = Common::get_iface_ip("lo");
    std::cout << ip_address << '\n';
}
