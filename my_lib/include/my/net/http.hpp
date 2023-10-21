//
// Created by wwww on 2023/8/23.
//

#ifndef CPP_SIMPLE_WEB_SERVER_HTTP_HPP
#define CPP_SIMPLE_WEB_SERVER_HTTP_HPP

#include "rpc.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <numeric>
#include <shared_mutex>
#include <string_view>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace my::net::http {
using RpcFuncTable = std::unordered_map<std::string, rpc::HandlerType>;
static constexpr std::string_view default_html_dir = "/var/www/html";
static constexpr std::string_view default_index_page_name = "index.html";

class Reactor;
enum class ResponseCode : short {
  OK = 200,
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_SERVER_ERROR = 500
};

static std::unordered_map<ResponseCode, std::string_view> http_code_to_title{
    {ResponseCode::BAD_REQUEST, "Bad Request"},
    {ResponseCode::OK, "OK"},
    {ResponseCode::NOT_FOUND, "Not Found"},
    {ResponseCode::FORBIDDEN, "Forbidden"},
    {ResponseCode::INTERNAL_SERVER_ERROR, "Internal Error"}};

static std::unordered_map<ResponseCode, std::string_view> http_code_to_body{
    {ResponseCode::BAD_REQUEST,
     "<html><body><h1>400 - Your request has bad syntax or is inherently "
     "impossible to satisfy.</h1></body></html>"},
    {ResponseCode::NOT_FOUND, "<html><body><h1>404 - The requested file was "
                              "not fount on this server.</h1></body></html>"},
    {ResponseCode::FORBIDDEN,
     "<html><body><h1>403 - You do not have permission to get file from this "
     "server.</h1></body></html>"},
    {ResponseCode::INTERNAL_SERVER_ERROR,
     "<html><body><h1>500 - There was an unusual problem serving the requested "
     "file.</h1></body></html>"}};

enum class IOState { OK, PENDING, BAD, KEEP_ALIVE };
struct Request {
  std::string_view url, version, host, method, content;
  size_t content_length{0};
  bool keep_alive{false};
};

class RequestParser {
private:
  Request m_request{};
  size_t m_pivot{0};
  enum class ProtocolState {
    ON_REQUEST_LINE,
    ON_HEADERS,
    ON_CONTENT
  } m_protocol_state{ProtocolState::ON_REQUEST_LINE};
  IOState parse_request_line(std::string_view);
  IOState parse_header_line(std::string_view);
  IOState parse_content(std::string_view);
  std::pair<IOState, std::string_view> next_line(std::string_view);

public:
  inline const Request &unwrap() { return m_request; };
  IOState update(std::string_view);
};

struct ResponseBuffer {
  ResponseCode code{ResponseCode::OK};
  int file_fd{-1};
  std::string s{};
  size_t write_index{0};
  ssize_t file_size{0}, file_write_index{0};
};

class EpollSelector {
private:
  constexpr static int MAX_EVENT_NUM = 10000;
  std::array<epoll_event, MAX_EVENT_NUM> m_events{};
  int m_timeout{-1};
  int m_epoll_fd, m_listen_fd;
  size_t m_current_event_num{0}, m_current_event_index{0};
  static int set_nonblocking_fd(int fd);

public:
  struct Event {
    enum class Tag { CONNECTION, READ, WRITE, CLOSE } tag;
    int fd;
  };

  void register_on_listening_lt(int fd) const;
  void register_on_reading(int fd, bool one_shot = true,
                           bool blocking = false) const;
  void register_timer(int fd);
  void read_again_on(int fd) const;
  void write_again_on(int fd) const;
  // remove and close socket
  void unregister(int fd) const;
  Event get_next_event();

  explicit EpollSelector(int listen_fd, std::size_t size = 5);
  EpollSelector() = delete;
  ~EpollSelector();
};

// read -> work -> write
class Handler {
private:
  constexpr static size_t READ_BUFFER_SIZE = 2048;
  struct sockaddr_in m_addr;
  std::string m_read_buffer;
  size_t m_read_index;
  RequestParser m_request_parser;
  ResponseBuffer m_response_buffer;
  bool m_keep_alive;
  std::chrono::steady_clock::time_point m_last_alive_time, m_lazy_current_time;

public:
  friend class Reactor;
  explicit Handler(std::chrono::steady_clock::time_point, const sockaddr_in &);
  [[nodiscard]] std::string get_addr_str() const;
  IOState read(int fd);
  IOState work(std::string_view html_dir, RpcFuncTable  &);
  IOState write(int fd);
  void clear();
  void update_current_time(std::chrono::steady_clock::time_point);
};

class Acceptor {
private:
  int m_listen_fd;

public:
  [[nodiscard]] std::tuple<int, struct sockaddr_in> accept() const;
  explicit Acceptor(int fd);
  Acceptor() = default;
};

class Reactor {
public:
  constexpr static inline size_t MAX_FD = 65536;
  struct Config {
    std::string_view ip;
    int port;
    std::string_view mapping_path = default_html_dir;
    size_t working_thread_num = 4, max_idle_seconds = 30, listen_size = 5,
           selector_size = 5;
  };

private:
  std::vector<std::shared_ptr<Handler>> m_handlers;
  Config m_config;
  int m_server_fd;

//public:
  RpcFuncTable m_rpc_funcs;

public:
  explicit Reactor(const Config &);
  void run();

  ~Reactor();
  template <typename Func>
  bool rpc_register(std::string const &name, Func func) {
    auto url = std::string{rpc::default_route_prefix} + name;
    if (m_rpc_funcs.find(url) != m_rpc_funcs.end())
      return false;
    m_rpc_funcs[url] = [f = func](std::string_view content) -> std::string {
      using namespace MyJson;
      auto j = Json::from_json_text(content);
      if (!j.has_value())
        return R"({"rpc_error":"broken client json"})";
      using ArgT =
          std::remove_cv_t<std::remove_reference_t<rpc::FirstArgT<Func>>>;
//      static_assert(std::is_same_v<ArgT, int>);
      auto arg = j.value().to_type<ArgT>();
      if (!arg.has_value())
        return R"({"rpc_error":"broken client arg"})";
      return Json{f(arg.value())}.to_json_text();
    };
    return true;
  }
};
} // namespace my::net::http
#endif // CPP_SIMPLE_WEB_SERVER_HTTP_HPP
