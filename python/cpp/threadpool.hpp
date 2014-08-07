#pragma once

#include "comms/minion.hpp"

#include <thread>

void funcWrapper(py::object &func)
{
  func();
}

class ThreadPool
{
  public:
    void run(py::object &func)
    {
      futures_.push_back(std::move(std::async(std::launch::async,
        funcWrapper, std::ref(func))));
    }

    void wait()
    {
      for (auto &future : futures_)
        future.wait();
    }
    
  private:
    std::vector<std::future<void>> futures_;
};

void exportThreadPool()
{
  py::class_<ThreadPool, boost::noncopyable>("ThreadPool")
    .def("run", &ThreadPool::run)
    .def("wait", &ThreadPool::wait);
}
