//!
//! Contains the implementation of the worker.
//!
//! \file comms/worker.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/worker.hpp"
#include "comms/thread.hpp"

#include <cstdlib>
#include <easylogging/easylogging++.h>

namespace stateline { namespace comms {

Worker::Worker(zmq::context_t& ctx, const WorkerSettings& settings, bool& running)
  : minion_{ctx, zmq::socket_type::dealer, "toMinion", NO_LINGER}
  , network_{ctx, zmq::socket_type::dealer, "toNetwork", NO_LINGER}
  , router_{"worker", std::tie(minion_, network_)}
  , hbSettings_{settings.heartbeat}
  , running_{running}
  , minionWaiting_{true}
{
  // Initialise the local sockets
  minion_.bind(settings.workerAddress);
  network_.setIdentity();
  LOG(INFO) << "Worker connecting to " << settings.networkAddress;
  network_.connect("tcp://" + settings.networkAddress);

  // Specify the Worker functionality
  auto onJobFromNetwork = [&] (const Message& m) {
    if (minionWaiting_)
    {
      minion_.send(m);
      minionWaiting_ = false;
    }
    else
    {
      queue_.push(m);
    }
  };

  auto onResultFromMinion = [&] (const Message & m)
  {
    network_.send(m);
    if (queue_.size() > 0)
    {
      minion_.send(queue_.front());
      queue_.pop();
    }
    else
    {
      minionWaiting_ = true;
    }
  };

  auto forwardToHB = [&](const Message& m) { };
  auto forwardToNetwork = [&](const Message& m) { network_.send({{},m.subject, m.data}); };
  /*auto disconnect = [&](const Message&)
  {
    LOG(INFO)<< "Worker disconnecting from server";
    exit(EXIT_SUCCESS);
  };*/

  // Just the order we gave them to the router
  const uint MINION_SOCKET = 0, NETWORK_SOCKET = 1;

  // Bind functionality to the router
  router_.bind<MINION_SOCKET, RESULT>(onResultFromMinion);
  router_.bind<MINION_SOCKET, HELLO>(forwardToNetwork);
  router_.bind<NETWORK_SOCKET, JOB>(onJobFromNetwork);
  router_.bind<NETWORK_SOCKET, HEARTBEAT>(forwardToHB);
  router_.bind<NETWORK_SOCKET, HELLO>(forwardToHB);
  router_.bind<NETWORK_SOCKET, BYE>(forwardToHB);
  //router_.bindOnPoll(sendHeartbeat);
}

void Worker::start()
{
  router_.poll(msPollRate_, running_);
}

}

}
