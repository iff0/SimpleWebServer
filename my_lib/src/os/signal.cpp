//
// Created by wwww on 2023/8/23.
//

#include <csignal>
#include <cstring>
#include <my/os/signal.hpp>
#include <stdexcept>
namespace my::os {
void handle_signal(int sig, void (*handler)(int), bool restart) {
  struct sigaction sa {};
  std::memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  if (restart)
    sa.sa_flags |= SA_RESTART;
  sigfillset(&sa.sa_mask);
  if (auto ret = sigaction(sig, &sa, nullptr); ret != 0)
    throw std::runtime_error{std::string{"Bad signal sigaction, sig="} +
                             std::to_string(sig)};
}
} // namespace my::os