#pragma once

#include <easylogging/easylogging++.h>

// Notes, this might have be be done on a per-type basis because the args 
// don't line up. Otherwise I may have to restict what args the delegator,
// worker, and heartbeat systems all take.

namespace stateline
{
  template <class T, class...Args>
  std::future<bool> startInThread(bool& running, Args&&... args)
  {
    auto func = [&running](Args&&... args) -> bool
    {
      try
      {
        T obj(std::forward<Args>(args)..., std::ref(running));
        obj.start();
      }
      catch (const zmq::error_t& ex)
      {
        // Ignore ZMQ errors to prevent ugly stack trace
        LOG(INFO) << "Caught interrupt. Goodbye!";
      }
      catch (const std::exception& ex)
      {
        LOG(FATAL) << "Exception thrown in child thread: " << ex.what();
      }
      return true;
    };

    return std::async(std::launch::async, func, std::forward<Args>(args)...);
  }
}
