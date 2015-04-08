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
#include "comms/serial.hpp"
#include "comms/thread.hpp"

#include <cstdlib>

namespace stateline
{
  namespace comms
  {

    Worker::Worker(zmq::context_t& context, const WorkerSettings& settings)
      : minion_(context, ZMQ_ROUTER, "toMinion"),
        heartbeat_(context, ZMQ_PAIR, "toHBRouter"),
        network_(context, ZMQ_DEALER, "toNetwork"),
        router_("main", {&minion_, &heartbeat_, &network_}),
        running_(true)
    {
      // Initialise the local sockets
      minion_.bind(WORKER_SOCKET_ADDR);
      heartbeat_.bind(CLIENT_HB_SOCKET_ADDR);
      network_.setIdentifier(randomSocketID());
      network_.connect("tcp://" + settings.address);

      LOG(INFO) << "Worker connecting to " << address;

      // Specify the Worker functionality
      auto forwardToMinion = [&](const Message&m) { minion_.send(m); };
      auto forwardToHB = [&](const Message& m) { heartbeat_.send(m); };
      auto fForwardToNetwork = [&](const Message& m) { network_.send(m); };
      auto fDisconnect = [&](const Message& m)
      {
        LOG(INFO)<< "Worker disconnecting from server";
        exit(EXIT_SUCCESS);
      };

      // Just the order we gave them to the router
      const uint MINION_SOCKET=0,HB_SOCKET=1,NETWORK_SOCKET=2;

      // Bind functionality to the router
      router_.bind(MINION_SOCKET, WORK, forwardToNetwork);
      router_.bind(HB_SOCKET, HEARTBEAT, forwardToNetwork);
      router_.bind(HB_SOCKET, GOODBYE, disconnect);
      router_.bind(NETWORK_SOCKET, WORK, forwardToMinion);
      router_.bind(NETWORK_SOCKET, HEARTBEAT, forwardToHB);
      router_.bind(NETWORK_SOCKET, HELLO, forwardToHB);
      router_.bind(NETWORK_SOCKET, GOODBYE, forwardToHB);

      // Initialise the connection
      LOG(INFO)<< "Connecting to server...";
      network_.send(Message(HELLO));
      // Should be a Hello back from the delegator
      Message reply = network_.receive();
      // TODO: explicit time-out here and error
      LOG(INFO)<< "Connection to server initialised";

      // Start the heartbeat thread
      // TODO this has to use our templated thread starter
      // TODO should we store the future returned by this function
      // so we can wait for the client heartbeat to finish when we terminate?
      startInThread<ClientHeartbeat>(context_, settings.heartbeat);
      // heartbeat_ = new ClientHeartbeat(*context_, settings.heartbeat);

      // Start the router and heartbeating
      router_.poll(settings.msPollRate, running_);
    }

    Worker::~Worker()
    {
      running_ = false;
    }

  }
}
