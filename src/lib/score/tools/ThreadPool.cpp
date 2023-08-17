#include <score/tools/ThreadPool.hpp>
#include <score/application/ApplicationServices.hpp>
#include <thread>
#if __has_include(<sys/resource.h>)
#include <sys/resource.h>
#endif
#include <ossia/detail/thread.hpp>

namespace score
{
ThreadPool::ThreadPool() { }

ThreadPool::~ThreadPool()
{
  if(m_threads)
  {
    for(int i = 0; i < m_numThreads; i++)
    {
      m_threads[i].quit();
    }
    for(int i = 0; i < m_numThreads; i++)
    {
      m_threads[i].wait();
    }

    m_threads.reset();
  }
}

ThreadPool& ThreadPool::instance()
{
  static std::once_flag init{};
  std::call_once(init, [] {
    score::AppServices().threadpool.emplace();
  });
  return *score::AppServices().threadpool;
}

QThread* ThreadPool::acquireThread()
{
  if(!m_threads)
  {
    m_numThreads = std::thread::hardware_concurrency();
    if(m_numThreads > 2)
      m_numThreads = m_numThreads / 2;
    if(m_numThreads < 2)
      m_numThreads = 2;
    m_threads = std::make_unique<QThread[]>(m_numThreads);

    for(int i = 0; i < m_numThreads; i++)
    {
#if __has_include(<sys/resource.h>)
      ::rlimit lim{0, 0};
      getrlimit(RLIMIT_STACK, &lim);

      if(lim.rlim_cur > m_threads[i].stackSize())
        m_threads[i].setStackSize(lim.rlim_cur);
#endif
      m_threads[i].setObjectName(QString("ossia uitask %1").arg(i));
      m_threads[i].start();
    }
    m_currentThread = 0;
  }

  QThread& t = m_threads[m_currentThread];
  m_currentThread++;
  m_currentThread = m_currentThread % m_numThreads;
  m_inFlight++;
  return &t;
}

void ThreadPool::releaseThread()
{
  m_inFlight--;

  if(m_inFlight == 0)
  {
    for(int i = 0; i < m_numThreads; i++)
    {
      m_threads[i].quit();
    }
    for(int i = 0; i < m_numThreads; i++)
    {
      m_threads[i].wait();
    }

    m_threads.reset();
  }
}

TaskPool::TaskPool()
{
  m_running = true;
  int i = 0;
  for(auto& t : m_threads)
  {
    t = std::thread{[this, i = i++] {
      ossia::set_thread_name("ossia task " + std::to_string(i));
      while(m_running)
      {
        task t{};
        if(m_queue.wait_dequeue_timed(t, 100000))
        {
          t();
        }
      }
    }};
  }
}

TaskPool::~TaskPool()
{
  m_running = false;
  for(auto& t : m_threads)
  {
    t.join();
  }
}

TaskPool& TaskPool::instance()
{
  static std::once_flag init{};
  std::call_once(init, [] {
    score::AppServices().taskpool.emplace();
  });
  return *score::AppServices().taskpool;
}

}
