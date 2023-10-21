//
// Created by wwww on 2023/10/17.
//
#ifndef CPP_SIMPLE_WEB_SERVER_RPC_HPP
#define CPP_SIMPLE_WEB_SERVER_RPC_HPP
#include "my_json/core.hpp"
#include "type_traits"
#include <functional>
#include <netinet/in.h>
#include <string_view>

namespace my::net::rpc::detail {
template <typename T> struct arg_type;
template <typename R, typename Arg> struct arg_type<R(Arg)> {
  using type = Arg;
};
// function pointer
template <typename R, typename Arg> struct arg_type<R (*)(Arg)> {
  using type = Arg;
};

// member function pointer
template <typename C, typename R, typename Arg> struct arg_type<R (C::*)(Arg)> {
  using type = Arg;
};

// const member function pointer
template <typename C, typename R, typename Arg>
struct arg_type<R (C::*)(Arg) const> {
  using type = Arg;
};
template <typename F, typename = void> struct first_type {
  using type = typename arg_type<F>::type;
};
template <typename F>
struct first_type<F, std::enable_if_t<std::is_class_v<F>>> {
  using type = typename arg_type<decltype(&F::operator())>::type;
};
} // namespace my::net::rpc::detail

namespace my::net::rpc {
static std::string_view default_route_prefix = "/rpc/";
using HandlerType = std::function<std::string(std::string_view)>;
// struct RpcError {};
struct Client {
private:
  std::string m_ip;
  int m_port;
  int m_sock_fd;
  sockaddr_in m_serv_addr{};
  std::string m_response_buffer;

public:
  bool dial(const std::string &address);
  template <typename ArgT, typename ResT>
  std::optional<ResT> call(std::string const &name, ArgT const &arg) {
    if ((m_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      std::cout << "Rpc TCP Socket creation error" << std::endl;
      return {};
    }
    if (connect(m_sock_fd, (struct sockaddr *)&m_serv_addr,
                sizeof(m_serv_addr)) < 0) {
      std::cout << "Rpc Connection failed" << std::endl;
      return {};
    }
    std::string req = "POST ";
    req += default_route_prefix;
    req += name;
    req += " HTTP/1.1\r\n";
    auto encoded = MyJson::Json{arg}.to_json_text();
    req += "Content-Length: ";
    req += std::to_string(encoded.size());
    req += "\r\n\r\n";
    req += encoded;
    ssize_t sent_pivot = 0;
    while (sent_pivot < req.size()) {
      auto adder =
          send(m_sock_fd, req.data() + sent_pivot, req.size() - sent_pivot, 0);
      if (adder <= 0) {
        std::cout << "Rpc Server Closed\n";
        return {};
      }
      sent_pivot += adder;
    }
    m_response_buffer.resize(1024);
    ssize_t read_pivot = 0;
    while (true) {
      if (read_pivot >= m_response_buffer.size())
        m_response_buffer.resize(int(1.25 * m_response_buffer.size()));
      auto adder = recv(m_sock_fd, m_response_buffer.data() + read_pivot,
                        m_response_buffer.size() - read_pivot, 0);
      if (adder < 0) {
        std::cout << "Rpc Server Closed\n";
        return {};
      } else if (adder == 0)
        break;
      read_pivot += adder;
    }
    m_response_buffer.resize(read_pivot);
    std::size_t rep_begin = 0;
    while (rep_begin < m_response_buffer.size()) {
      if (rep_begin + 3 < m_response_buffer.size() &&
          std::string_view{m_response_buffer.data() + rep_begin, 4} ==
              "\r\n\r\n") {

        break;
      }
      ++rep_begin;
    }
    if (rep_begin + 3 >= m_response_buffer.size()) {

      std::cout << "Rpc bad response:\n " << m_response_buffer;
      return {};
    }
    auto content = std::string_view{m_response_buffer.data() + rep_begin + 4,
                                    m_response_buffer.size() - rep_begin - 4};
    auto j = MyJson::Json::from_json_text(content);
    if (!j.has_value()) {
      std::cout << "Rpc bad response:\n "<< m_response_buffer;
      return {};
    }
    auto decoded = j->to_type<ResT>();
    if (decoded.has_value())
      return decoded.value();
    std::cout << "Rpc bad response:\n " << m_response_buffer;
    return {};
  }
};
template <typename F> using FirstArgT = typename detail::first_type<F>::type;

} // namespace my::net::rpc
#endif // CPP_SIMPLE_WEB_SERVER_RPC_HPP
