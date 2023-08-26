//
// Created by wwww on 2023/8/25.
//
#include <my/os.hpp>
#include <stdexcept>
#include <sys/timerfd.h>
namespace my::os {

Timer::Timer(std::size_t seconds, std::size_t nanoseconds) {
  struct itimerspec timer_opt {};
  m_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (m_fd < 0)
    throw std::runtime_error{"cannot create timer"};

  timer_opt.it_value.tv_sec = seconds;
  timer_opt.it_value.tv_nsec = nanoseconds;
  timer_opt.it_interval.tv_sec = seconds;
  timer_opt.it_interval.tv_nsec = nanoseconds;

  if (timerfd_settime(m_fd, 0, &timer_opt, nullptr) < 0)
    throw std::runtime_error{"cannot create timer"};
}

std::uint64_t Timer::tick() const {
  std::uint64_t count = 0;
  read(m_fd, (void *)&count, sizeof(count));
  return count;
}
} // namespace my::os