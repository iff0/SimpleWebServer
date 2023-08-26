//
// Created by wwww on 2023/8/24.
//
#include <fmt/core.h>
#include <my/net/http.hpp>
#include <my/net/tcp.hpp>
#include <my/os.hpp>
#include <my/thread.hpp>
#include <unistd.h>
constexpr auto DEBUG = false;

using namespace std::chrono_literals;

namespace my::net::http {

Reactor::Reactor(const Reactor::Config &cfg) : m_config{cfg} {

  m_server_fd = tcp::create_socket();
  tcp::bind(m_server_fd, cfg.ip, cfg.port);
  tcp::listen(m_server_fd, cfg.listen_size);
  fmt::println("[INFO] Server listening on {}:{}", cfg.ip, cfg.port);
}
Reactor::~Reactor() {
  close(m_server_fd);
  for (auto const &kv : m_handlers)
    close(kv.first);
}

auto Reactor::run() -> void {
  auto selector = EpollSelector(m_server_fd, m_config.selector_size);
  auto acceptor = Acceptor(m_server_fd);
  auto close_connection = [&](int fd) {
    selector.unregister(fd);
    m_handlers.erase(fd);
    close(fd);
  };

  auto add_connection = [&](int client_fd, const Handler &handler) {
    selector.register_on_reading(client_fd);
    m_handlers.emplace(client_fd, handler);
  };

  selector.register_on_reading(m_server_fd, false);
  auto thread_pool = thread::ThreadPool(m_config.working_thread_num);

  auto max_connection_idle_time = 30s;
  auto idle_timer = my::os::Timer(max_connection_idle_time.count(), 0);
  auto idle_timer_fd = idle_timer.get_fd();
  selector.register_timer(idle_timer_fd);

  std::shared_mutex worker_mutex;

  auto remove_idle_connections = [&] {
    std::unique_lock lg(worker_mutex, std::defer_lock);

    // if there are  some handlers still working, dont remove handler to avoid -
    // dangling reference!
    if (!lg.try_lock())
      return;
    std::vector<int> idle_connections;
    for (auto &[k, v] : m_handlers)
      if (v.m_last_alive_time + max_connection_idle_time <
          std::chrono::steady_clock::now())
        idle_connections.emplace_back(k);
    for (auto fd : idle_connections)
      close_connection(fd);
  };

  auto done = false;
  while (!done) {
    using EventTag = EpollSelector::Event::Tag;
    auto event = selector.get_next_event();
    auto [tag, fd] = event;
    if (tag == EventTag::CONNECTION) {
      // incoming request
      auto handler = acceptor.accept();
      auto client_fd = handler.m_client_fd;
      if constexpr (DEBUG) {
        fmt::println("[INFO] hello from {}  on fd {}", handler.get_addr_str(),
                     client_fd);
      }
      add_connection(client_fd, handler);
      continue;
    }
    if (fd == idle_timer_fd) {
      //      fmt::println("tick {}", idle_timer.tick());
      idle_timer.tick();
      remove_idle_connections();
      continue;
    }
    // maybe its a idled keep-alive that i killed some short seconds before
    // (removed by remove_idle_connections before being looped at)
    if (m_handlers.find(fd) == m_handlers.end())
      continue;
    auto &handler = m_handlers.at(fd);

    if (tag == EventTag::CLOSE) {
      if constexpr (DEBUG) {
        fmt::println("[INFO] bye to {}  on fd {}", handler.get_addr_str(),
                     handler.m_client_fd);
      }
      close_connection(fd);
    } else if (tag == EventTag::READ) {
      auto state = handler.read();
      if (state != IOState::OK) {
        close_connection(fd);
        continue;
      }
      thread_pool.submit([&, fd = fd]() {
        std::shared_lock lg{worker_mutex};
        if (m_handlers.find(fd) == m_handlers.end())
          return;
        auto& handler = m_handlers.at(fd);
        auto state = handler.work(m_config.mapping_path);
        if (state == IOState::PENDING)
          selector.read_again_on(fd);
        else
          selector.write_again_on(fd);
      });
    } else if (tag == EventTag::WRITE) {
      auto state = handler.write();
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