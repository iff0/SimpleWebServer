//
// Created by wwww on 2023/8/23.
//
#include <cerrno>
#include <fmt/format.h>
#include <my/net/http.hpp>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

#include <sys/fcntl.h>
namespace my::net::http {

EpollSelector::EpollSelector(int listen_fd, size_t size)
    : m_listen_fd{listen_fd} {
  m_epoll_fd = epoll_create(size);
}

EpollSelector::~EpollSelector() { close(m_epoll_fd); }

auto set_nonblocking_fd(int fd) -> int {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

auto EpollSelector::set_nonblocking_fd(int fd) -> int {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

auto EpollSelector::register_timer(int fd) -> void {

  epoll_event event{.events = EPOLLIN | EPOLLERR | EPOLLHUP,
                    .data = {.fd = fd}};
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
  set_nonblocking_fd(fd);
}

void EpollSelector::register_on_reading(int fd, bool one_shot,
                                        bool blocking) const {
  epoll_event event{.events = EPOLLIN | EPOLLET | EPOLLRDHUP,
                    .data = {.fd = fd}};
  if (one_shot)
    event.events |= EPOLLONESHOT;
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
  if (!blocking)
    set_nonblocking_fd(fd);
}

auto EpollSelector::unregister(int fd) const -> void {
  epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

auto EpollSelector::read_again_on(int fd) const -> void {
  epoll_event event{.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP,
                    .data = {.fd = fd}};
  epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

auto EpollSelector::write_again_on(int fd) const -> void {
  epoll_event event{.events = EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP,
                    .data = {.fd = fd}};
  epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

auto EpollSelector::get_next_event() -> Event {
  while (m_current_event_index >= m_current_event_num) {
    m_current_event_index = 0;
    m_current_event_num =
        epoll_wait(m_epoll_fd, m_events.data(), MAX_EVENT_NUM, m_timeout);
    if (m_current_event_num < 0 && (errno != EINTR)) {
      auto err = errno;
      throw std::runtime_error{
          fmt::format("bad epoll trigger, errno = {}", err)};
    }
  }
  auto const &e = m_events[m_current_event_index++];
  auto fd = e.data.fd;
  auto event = e.events;
  if (fd == m_listen_fd)
    return {Event::Tag::CONNECTION, {}};
  if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    return {Event::Tag::CLOSE, fd};
  if (event & EPOLLIN)
    return {Event::Tag::READ, fd};
  if (event & EPOLLOUT)
    return {Event::Tag::WRITE, fd};
  throw std::runtime_error{
      fmt::format("unknown epoll event type, event={}", event)};
}

} // namespace my::net::http