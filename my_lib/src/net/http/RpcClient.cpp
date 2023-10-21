//
// Created by wwww on 2023/10/17.
//
#include <arpa/inet.h>
#include <my/net/rpc.hpp>
#include <sys/socket.h>
#include <unistd.h>
namespace my::net::rpc {

bool Client::dial(const std::string &address) {
  std::stringstream ss(address);
  std::getline(ss, m_ip, ':');
  if (!ss.eof()) {
    ss >> m_port;
  } else {
    m_port = 80; // Default port
  }

  m_serv_addr.sin_family = AF_INET;
  m_serv_addr.sin_port = htons(m_port);
  if (inet_pton(AF_INET, m_ip.data(), &m_serv_addr.sin_addr) <= 0) {
    std::cout << "Invalid address or Address not supported" << std::endl;
    return false;
  }


  return true;
}


} // namespace my::net::rpc