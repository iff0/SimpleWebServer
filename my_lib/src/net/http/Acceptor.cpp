//
// Created by wwww on 2023/8/24.
//
#include <my/net/http.hpp>
#include <my/net/tcp.hpp>
namespace my::net::http {

Acceptor::Acceptor(int fd) : m_listen_fd{fd} {}

auto Acceptor::accept() const -> std::tuple<int ,struct sockaddr_in> {
  return my::net::tcp::accept(m_listen_fd);
}
} // namespace my::net::http