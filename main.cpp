//
// Created by wwww on 2023/4/11.
//
#include <csignal>
#include <fmt/core.h>
#include <iostream>
#include <my/net.hpp>
#include <my/os.hpp>

using namespace my;
using namespace std::chrono_literals;

auto main(int argc, char *argv[]) -> int {

  std::string_view html_root_dir = net::http::default_html_dir;

  char *ip;
  int port;
  std::size_t working_thread_num;
  try {
    if (argc < 4 || argc > 5)
      throw std::invalid_argument{"bad argc"};
    ip = argv[1];
    port = std::stoi(argv[2]);
    working_thread_num = std::stoi(argv[3]);
    if (argc > 4)
      html_root_dir = argv[4];
  } catch (...) {
    fmt::print(
        "usage: {} ip_address port_number threads_num [index_page_path]\n",
        argv[0]);
    return 0;
  }

  my::os::handle_signal(SIGPIPE, SIG_IGN);

  net::http::Reactor::Config config{.ip = ip,
                                    .port = port,
                                    .mapping_path = html_root_dir,
                                    .working_thread_num = working_thread_num,
                                    .max_idle_seconds = 30,
                                    .listen_size = 5};
  auto server = net::http::Reactor(config);
  server.run();
}