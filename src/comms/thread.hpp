#pragma once

// Notes, this might have be be done on a per-type basis because the args 
// don't line up. Otherwise I may have to restict what args the delegator,
// worker, and heartbeat systems all take.

template <class T, class...Args>
std::future<bool> startInThread(std::reference_wrapper<bool> running, Args&&... args)
{
  auto func = [=](Args&&... args) -> bool
  {
    try
    {
      T obj(std::forward<Args>(args)..., running);
      obj.start();
    }
    catch (const std::exception& ex)
    {
      LOG(FATAL) << "Exception thrown in child thread: " << ex.what();
    }
    return true;
  };

  return std::async(std::launch::async, func, std::forward<Args>(args)...);
}
