#include "socket_utils.h"

namespace common {

    std::string get_iface_ip(const std::string& iface) {
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
    bool set_non_blocking(int fd) {
        const auto flags = fcntl(fd, F_GETFL, 0); // this returns the flags associated with fd currently
        if (flags == -1) return false;
        if (flags & O_NONBLOCK) return true;
        return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    // disabling Nagle's algorithm
    bool set_no_delay(int fd) {
        int one = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<void *>(&one), sizeof(one)) != -1);
    }

    














}












int main() {
    auto ip_address = common::get_iface_ip("lo");
    std::cout << ip_address << '\n';
}
