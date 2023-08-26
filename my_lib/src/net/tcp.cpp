//
// Created by wwww on 2023/8/23.
//
#include <arpa/inet.h>
#include <fmt/format.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
namespace my::net::tcp {
auto create_socket() -> int {
  auto fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    throw std::runtime_error{"cannot create new tcp socket"};
  return fd;
}

auto bind(int fd, std::string_view ip, size_t port) -> void {
  struct sockaddr_in address {};
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  inet_pton(AF_INET, ip.data(), &address.sin_addr);
  address.sin_port = htons(port);
  auto ret = bind(fd, (struct sockaddr *)&address, sizeof address);
  if (ret != 0)
    throw std::runtime_error{fmt::format("cannot bind on {}:{}", ip, port)};
}

auto listen(int fd, size_t n) -> void {
  auto ret = ::listen(fd, n);
  if (ret != 0)
    throw std::runtime_error{"bad listen"};
}

auto accept(int listen_fd) -> std::tuple<int, struct sockaddr_in> {
  struct sockaddr_in client_address {};
  socklen_t client_addr_length = sizeof(client_address);
  int fd = accept(listen_fd, (struct sockaddr *)&client_address,
                  &client_addr_length);
  if (fd < 0) {
    auto err = errno;
    std::cout << err << std::endl;
    throw std::runtime_error{"bad accept"};
  }
  return {fd, client_address};
}
} // namespace my::net::tcp