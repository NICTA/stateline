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

namespace stateline
{
  namespace comms
  {

    Worker::Worker(zmq::context_t& context, const WorkerSettings& settings, bool& running)
      : context_(context),
        minion_(context, ZMQ_DEALER, "toMinion"),
        heartbeat_(context, ZMQ_PAIR, "toHBRouter"),
        network_(context, ZMQ_DEALER, "toNetwork"),
        router_("main", {&minion_, &heartbeat_, &network_}),
        msPollRate_(settings.msPollRate),
        hbSettings_(settings.heartbeat),
        running_(running),
        minionWaiting_(true)
    {
      // Initialise the local sockets
      minion_.bind(settings.workerAddress);
      heartbeat_.bind(CLIENT_HB_SOCKET_ADDR);
      network_.setIdentifier();
      LOG(INFO) << "Worker connecting to " << settings.networkAddress;
      network_.connect("tcp://" + settings.networkAddress);

      // Specify the Worker functionality
      //
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


      auto forwardToHB = [&](const Message& m) { heartbeat_.send(m); };
      auto forwardToNetwork = [&](const Message& m) { network_.send({{},m.subject, m.data}); };
      auto disconnect = [&](const Message&)
      {
        LOG(INFO)<< "Worker disconnecting from server";
        exit(EXIT_SUCCESS);
      };

      // Just the order we gave them to the router
      const uint MINION_SOCKET=0,HB_SOCKET=1,NETWORK_SOCKET=2;

      // Bind functionality to the router
      router_.bind(MINION_SOCKET, RESULT, onResultFromMinion);
      router_.bind(MINION_SOCKET, HELLO, forwardToNetwork);
      router_.bind(HB_SOCKET, HEARTBEAT, forwardToNetwork);
      router_.bind(HB_SOCKET, GOODBYE, disconnect);
      router_.bind(NETWORK_SOCKET, JOB, onJobFromNetwork);
      router_.bind(NETWORK_SOCKET, HEARTBEAT, forwardToHB);
      router_.bind(NETWORK_SOCKET, HELLO, forwardToHB);
      router_.bind(NETWORK_SOCKET, GOODBYE, forwardToHB);
    }

    Worker::~Worker()
    {
    }

    void Worker::start()
    {
      // Start the heartbeat thread and router
      auto future = startInThread<ClientHeartbeat>(running_, std::ref(context_), std::cref(hbSettings_));
      router_.poll(msPollRate_, running_);
      future.wait();
    }

  }
}
