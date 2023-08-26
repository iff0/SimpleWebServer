//
// Created by wwww on 2023/8/23.
//

#ifndef CPP_SIMPLE_WEB_SERVER_JTHREADS_HPP
#define CPP_SIMPLE_WEB_SERVER_JTHREADS_HPP
#include <thread>
#include <vector>
namespace my::thread {
class JoinThreads {
  std::vector<std::thread> &m_threads;

public:
  explicit JoinThreads(std::vector<std::thread> &threads)
      : m_threads(threads) {}
  ~JoinThreads() {
    for (auto &t : m_threads)
      if (t.joinable())
        t.join();
  }
};
} // namespace my::thread

#endif // CPP_SIMPLE_WEB_SERVER_JTHREADS_HPP
