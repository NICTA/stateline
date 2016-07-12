//!
//! The worker object connects with a delegator, then forwards job requests
//! to a set of minions that actually perform the work. These minions then
//! return the worker results, which are forwarded back to the delegator.
//!
//! \file comms/worker.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <queue>

#include <zmq.hpp>

#include "settings.hpp"
#include "datatypes.hpp"
#include "router.hpp"
#include "socket.hpp"

namespace stateline
{

namespace comms
{

//! Worker object that takes jobs, forwards them to a minion
//! then receives results from the minion and send them back
//! to the delegator.
//!
class Worker
{
public:
  //! Construct a new worker that can handle multiple types of jobs.
  //!
  //! \param settings The configuration object.
  //!
  Worker(zmq::context_t& ctx, const WorkerSettings& settings, bool& running);

  // Workers can't be copied.
  Worker(const Worker &) = delete;

  void start();

private:
  Socket minion_;
  Socket network_;
  Router<Socket, Socket> router_;

  uint msPollRate_;
  HeartbeatSettings hbSettings_;

  bool& running_;

  std::queue<Message> queue_;
  bool minionWaiting_;
};

} // namespace comms

} // namespace stateline
