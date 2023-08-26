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
  if (argc < 3 || argc > 4) {
    fmt::print("usage: {} ip_address port_number --index_page_path\n", argv[0]);
    return 0;
  }
  std::string_view html_root_dir = net::http::default_html_dir;
  if (argc == 4)
    html_root_dir = argv[3];
  auto ip = argv[1];
  int port;
  try {
    port = std::stoi(argv[2]);
  } catch (...) {
    fmt::println("bad port number: {}", argv[2]);
    return 0;
  }

  my::os::handle_signal(SIGPIPE, SIG_IGN);

  net::http::Reactor::Config config{.ip = ip,
                                    .port = port,
                                    .mapping_path = html_root_dir,
                                    .working_thread_num = 4,
                                    .max_idle_seconds = 30};
  auto server = net::http::Reactor(config);
  server.run();
}