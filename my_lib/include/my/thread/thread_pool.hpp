//
// Created by wwww on 2023/8/23.
//

#ifndef CPP_SIMPLE_WEB_SERVER_THREAD_POOL_HPP
#define CPP_SIMPLE_WEB_SERVER_THREAD_POOL_HPP

#include <atomic>
#include <functional>
#include <my/thread/jthreads.hpp>
#include <my/thread/thread_safe_queue.hpp>
#include <thread>
namespace my::thread {
class ThreadPool {
  std::atomic_bool m_done;
  ThreadSafeQueue<std::function<void()>> m_work_queue;
  std::vector<std::thread> m_threads;
  size_t m_threads_count;
  JoinThreads m_joiner;
  void worker_thread() {
    while (!m_done) {
      std::function<void()> task;
      if (m_work_queue.try_pop(task))
        task();
      else
        std::this_thread::yield();
    }
  }

public:
  explicit ThreadPool(size_t size) : m_threads_count{size}, m_done{false}, m_joiner{m_threads} {
    try {
      for (unsigned i = 0; i < m_threads_count; ++i)
        m_threads.emplace_back(&ThreadPool::worker_thread, this);
    } catch (...) {
      m_done = true;
      throw;
    }
  }
  ~ThreadPool() { m_done = true; }
  template <typename FunctionType> void submit(FunctionType f) {
    m_work_queue.push(std::function<void()>(f));
  }
};
} // namespace my::thread
#endif // CPP_SIMPLE_WEB_SERVER_THREAD_POOL_HPP
