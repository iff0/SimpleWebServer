//
// Created by wwww on 2023/8/23.
//

#ifndef CPP_SIMPLE_WEB_SERVER_TCP_HPP
#define CPP_SIMPLE_WEB_SERVER_TCP_HPP
#include <netinet/in.h>
namespace my::net::tcp {
// create some tcp socket file descriptor using sys/socket.h
int create_socket();

// throw std::runtime_error when fails
void bind(int fd, std::string_view ip, size_t port);

// throw std::runtime_error when fails
void listen(int fd, size_t n = 5);

// throw std::runtime_error when fails
std::tuple<int, struct sockaddr_in> accept(int listen_fd);
} // namespace my::net
#endif // CPP_SIMPLE_WEB_SERVER_TCP_HPP
