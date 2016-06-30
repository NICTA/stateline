//! Contains the interface to the supervisor.
//!
//! \file comms/supervisor.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <queue>

#include <zmq.hpp>

#include "settings.hpp"
#include "messages.hpp"
#include "datatypes.hpp"
#include "router.hpp"
#include "socket.hpp"

namespace stateline { namespace comms {

//! A supervisor provides an interface between the delegator and worker.
//! It manages a set of workers running on the same host and forwards work
//! from the delegator to those workers.
//!
class Supervisor
{
public:
  //! Construct a new worker that can handle multiple types of jobs.
  //!
  //! \param settings The configuration object.
  //!
  Supervisor(zmq::context_t& ctx, const WorkerSettings& settings);

  Supervisor(const Supervisor&) = delete;
  Supervisor& operator=(const Supervisor&) = delete;

  //! Start the supervisor.
  void start(bool& running);

private:
  RawSocket minion_;
  Socket network_;
  Router<RawSocket, Socket> router_;

  uint msPollRate_;
  HeartbeatSettings hbSettings_;

  std::queue<Message> queue_;
  bool minionWaiting_;
};

} // namespace comms

} // namespace stateline
