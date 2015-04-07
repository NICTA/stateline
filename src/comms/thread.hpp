

// Notes, this might have be be done on a per-type basis because the args 
// don't line up. Otherwise I may have to restict what args the delegator,
// worker, and heartbeat systems all take.

template <class T>
bool startInThread(Args...)
{
  auto func = [] (<args go in here, must be copies, std::ref etc>)
  {
    T obj(Args);
    obj.start();
  }
  threadReturned_ = std::async(std::launch::async, func, Args...);
  return threadReturned_;
}
