//
// Created by wwww on 2023/8/24.
//
#include <arpa/inet.h>
#include <fmt/core.h>
#include <my/net/http.hpp>
#include <my/net/tcp.hpp>
#include <my/os.hpp>
#include <my/thread.hpp>
#include <unistd.h>
constexpr auto DEBUG = false;

using namespace std::chrono_literals;

namespace my::net::http {

Reactor::Reactor(const Reactor::Config &cfg)
    : m_config{cfg}, m_handlers(MAX_FD) {

  m_server_fd = tcp::create_socket();
//  struct linger tmp {
//    1, 1
//  };
//  setsockopt(m_server_fd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
  int flag = 1;
  setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
  tcp::bind(m_server_fd, cfg.ip, cfg.port);
  tcp::listen(m_server_fd, cfg.listen_size);
  fmt::println("[INFO] Server listening on {}:{}", cfg.ip, cfg.port);
}
Reactor::~Reactor() {
  close(m_server_fd);
  for (int i = 0; i < m_handlers.size(); ++i)
    if (m_handlers[i] != nullptr)
      close(i);
}

auto Reactor::run() -> void {

  auto lazy_current_time = std::chrono::steady_clock::now();
  auto selector = EpollSelector(m_server_fd, m_config.selector_size);
//  auto acceptor = Acceptor(m_server_fd);
  auto close_connection = [&](int fd) {
    m_handlers[fd].reset();
    selector.unregister(fd);
    close(fd);
  };

  auto add_connection = [&](int client_fd, const sockaddr_in &client_addr) {
    selector.register_on_reading(client_fd);
    auto &handler = m_handlers[client_fd] ;
    handler = std::make_shared<Handler>(lazy_current_time, client_addr);
  };

  selector.register_on_listening_lt(m_server_fd);
  auto thread_pool = thread::ThreadPool(m_config.working_thread_num);

  auto max_connection_idle_time =
      std::chrono::seconds{m_config.max_idle_seconds};
  auto idle_timer = my::os::Timer(max_connection_idle_time.count(), 0);
  auto idle_timer_fd = idle_timer.get_fd();
  selector.register_timer(idle_timer_fd);


  auto remove_idle_connections = [&] {
    // if there are  some handlers still working, dont remove handler to avoid -
    // dangling reference!
    lazy_current_time = std::chrono::steady_clock::now();
    for (int i = 0; i < m_handlers.size(); ++i) {
      if (m_handlers[i] == nullptr)
        continue;
      if (  m_handlers[i]->m_last_alive_time + max_connection_idle_time <
              lazy_current_time)
        close_connection(i);
      else
        m_handlers[i]->update_current_time(lazy_current_time);
    }
  };

  auto done = false;
  while (!done) {
    using EventTag = EpollSelector::Event::Tag;
    auto event = selector.get_next_event();
    auto [tag, fd] = event;
    if (tag == EventTag::CONNECTION) {
      // incoming request
      auto && [client_fd, client_addr] = my::net::tcp::accept(m_server_fd);

      add_connection(client_fd, client_addr);
      if constexpr (DEBUG) {
        fmt::println("[INFO] hello from {}:{} on fd {}",
                     std::string_view{inet_ntoa(client_addr.sin_addr)},
                     ntohs(client_addr.sin_port), client_fd);
      }
      continue;
    }
    if (fd == idle_timer_fd) {
//            fmt::println("tick {}", idle_timer.tick());
      idle_timer.tick();
      remove_idle_connections();
      continue;
    }
    // maybe its a idled keep-alive that i killed some short seconds before
    // (removed by remove_idle_connections before being looped at)

    auto &handler = m_handlers[fd];

    if (tag == EventTag::CLOSE) {
      if constexpr (DEBUG) {
        fmt::println("[INFO] bye to {}  on fd {}", handler->get_addr_str(), fd);
      }
      close_connection(fd);
    } else if (tag == EventTag::READ) {
      auto state = handler->read(fd);
      if (state != IOState::OK) {
        close_connection(fd);
        continue;
      }
      thread_pool.submit([ &, h = m_handlers[fd], fd = fd ]() {
        auto state = h->work(m_config.mapping_path);
        if (state == IOState::PENDING)
          selector.read_again_on(fd);
        else
          selector.write_again_on(fd);
      });
    } else if (tag == EventTag::WRITE) {
      auto state = handler->write(fd);
      if (state == IOState::PENDING)
        selector.write_again_on(fd);
      else if (state == IOState::KEEP_ALIVE)
        selector.read_again_on(fd);
      else
        close_connection(fd);
    }
  }
}
} // namespace my::net::http