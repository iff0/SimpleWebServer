//
// Created by wwww on 2023/8/23.
//

#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <fmt/core.h>
#include <my/net/http.hpp>
#include <netinet/in.h>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
namespace my::net::http {

auto Handler::get_addr_str() const -> std::string {
  return fmt::format("{}:{}", std::string_view{inet_ntoa(m_addr.sin_addr)},
                     ntohs(m_addr.sin_port));
}

auto Handler::read(int fd) -> IOState {
  m_last_alive_time = m_lazy_current_time;
  if (m_read_buffer.size() < READ_BUFFER_SIZE)
    m_read_buffer.resize(READ_BUFFER_SIZE);
  if (m_read_index >= READ_BUFFER_SIZE)
    return IOState::BAD;
  while (true) {
    auto bytes_read = recv(fd, m_read_buffer.data() + m_read_index,
                           READ_BUFFER_SIZE - m_read_index, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      return IOState::BAD;
    } else if (bytes_read == 0) {
      // client has closed gracefully :(
      return IOState::BAD;
    }
    m_read_index += bytes_read;
  }
  return IOState::OK;
}

auto Handler::work(std::string_view html_dir) -> IOState {
  // m_request =
  auto state = m_request_parser.update(
      std::string_view(m_read_buffer.data(), m_read_index));

  if (state == IOState::PENDING)
    return IOState::PENDING;
  else if (state == IOState::BAD)
    m_response_buffer.code = ResponseCode::BAD_REQUEST;
  auto &request = m_request_parser.unwrap();
  auto file_name = std::string{html_dir} + std::string{request.url};
  if (request.url.size() < 2)
    file_name += default_index_page_name;
  struct stat file_state {};
  // file dont exists

  m_response_buffer.code = [&] {
    if (stat(file_name.data(), &file_state) != 0)
      return ResponseCode::NOT_FOUND;
    if (S_ISDIR(file_state.st_mode)) {
      file_name += "/";
      file_name += default_index_page_name;
      if (stat(file_name.data(), &file_state) != 0)
        return ResponseCode::BAD_REQUEST;
    }
    // not readable for other group user
    if (!(file_state.st_mode & S_IROTH))
      return ResponseCode::FORBIDDEN;
    return ResponseCode::OK;
  }();
  if (m_response_buffer.code == ResponseCode::OK) {
    m_response_buffer.file_fd = open(file_name.data(), O_RDONLY);
    m_response_buffer.file_size = file_state.st_size;
    m_keep_alive = request.keep_alive;
  }
  if constexpr (true)
    fmt::println(
        R"([{}][{}] method="{}", url="{}", version="{}", host="{}", keep-alive={}, content-length={})",
        get_addr_str(), static_cast<short>(m_response_buffer.code),
        request.method, request.url, request.version, request.host,
        request.keep_alive, request.content_length);
  std::stringstream ss;
  auto code = m_response_buffer.code;
  ss << "HTTP/1.1 " << static_cast<short>(code) << " "
     << http_code_to_title[code] << "\r\n";
  ss << "Connection: " << (m_keep_alive ? "keep-alive" : "close") << "\r\n";
  ss << "Content-Length: "
     << (code == ResponseCode::OK ? file_state.st_size
                                  : http_code_to_body.at(code).size())
     << "\r\n";
  ss << "\r\n";
  if (code != ResponseCode::OK)
    ss << http_code_to_body.at(code);
  m_response_buffer.s = ss.str();
  return IOState::OK;
}

auto Handler::write(int fd) -> IOState {
  m_last_alive_time = m_lazy_current_time;
  //  fmt::println("response is", m_response_buffer.s);
  while (m_response_buffer.write_index < m_response_buffer.s.size()) {
    auto sent_bytes =
        send(fd, m_response_buffer.s.data() + m_response_buffer.write_index,
             m_response_buffer.s.size() - m_response_buffer.write_index, 0);
    if (sent_bytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return IOState::PENDING;
      return IOState::BAD;
    } else if (sent_bytes == 0)
      return IOState::BAD;
    m_response_buffer.write_index += sent_bytes;
  }
  if (m_response_buffer.file_fd != -1) {
    while (m_response_buffer.file_write_index < m_response_buffer.file_size) {
      auto sent_bytes = sendfile(
          fd, m_response_buffer.file_fd, &m_response_buffer.file_write_index,
          m_response_buffer.file_size - m_response_buffer.file_write_index);
      if (sent_bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          return IOState::PENDING;
        return IOState::BAD;
      } else if (sent_bytes == 0)
        return IOState::BAD;
    }
    close(m_response_buffer.file_fd);
  }
  if (m_keep_alive) {
    clear();
    return IOState::KEEP_ALIVE;
  }
  return IOState::OK;
}

auto Handler::clear() -> void {
  m_read_index = 0;
  m_request_parser = RequestParser();
  m_response_buffer = ResponseBuffer();
}

Handler::Handler(std::chrono::steady_clock::time_point t, const sockaddr_in & addr)
    : m_read_index{0}, m_request_parser(), m_response_buffer(),
      m_keep_alive{false}, m_last_alive_time{t}, m_lazy_current_time{t}, m_addr{addr} {

}
void Handler::update_current_time(
    std::chrono::steady_clock::time_point t) {
  m_lazy_current_time = t;
}

} // namespace my::net::http
