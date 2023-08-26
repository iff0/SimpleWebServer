//
// Created by wwww on 2023/8/23.
//

#ifndef CPP_SIMPLE_WEB_SERVER_OS_HPP
#define CPP_SIMPLE_WEB_SERVER_OS_HPP
#include <my/os/signal.hpp>
#include <cstdint>
#include <unistd.h>
namespace my::os {
class Timer {
private:
  int m_fd;

public:
  explicit Timer(std::size_t seconds, std::size_t nanoseconds = 0);
  [[nodiscard]] inline int get_fd() const { return m_fd; };
  ~Timer() { close(m_fd);}
  std::uint64_t tick() const;
};
} // namespace my::os
#endif // CPP_SIMPLE_WEB_SERVER_OS_HPP
