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

#include <glog/logging.h>
#include <zmq.hpp>

#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/datatypes.hpp"
#include "comms/router.hpp"
#include "comms/clientheartbeat.hpp"
#include "comms/socket.hpp"

namespace stateline
{
  namespace comms
  {
    //! Address that the minions connect their sockets to.
    // const std::string WORKER_SOCKET_ADDR = "inproc://worker";
    const std::string WORKER_SOCKET_ADDR = "ipc:///tmp/sl_worker.socket";

    //! Worker object that takes jobs, forwards them to a minion
    //! then receives results from the minion and send them back
    //! to the delegator.
    //!
    class Worker
    {
    public:
      //! Build a new worker that can handle multiple types of jobs.
      //!
      //! \param settings The configuration object.
      //!
      Worker(zmq::context_t& context, const WorkerSettings& settings, bool& running);

      // Workers can't be copied.
      Worker(const Worker &other) = delete;

      //! Destructor. Safely stops all polling threads and cleans up.
      //!
      ~Worker();

      void start();

    private:
      zmq::context_t& context_; 

      Socket minion_;
      Socket heartbeat_;
      Socket network_;
      SocketRouter router_;

      uint msPollRate_;
      HeartbeatSettings hbSettings_;

      bool& running_;

      std::queue<Message> queue_;
      bool minionWaiting_;
    };

    //! Forward a message to the delegator.
    //!
    //! \param m The message to forward.
    //! \param router The socket router.
    //!
    void forwardToNetwork(const Message& m, SocketRouter& router);

    //! Forward a message to a minion.
    //!
    //! \param m The message to forward.
    //! \param router The socket router.
    //!
    void forwardToMinion(const Message& m, SocketRouter& router);

    //! Disconnect from the server with a message.
    //!
    //! \param m The message to send to the server.
    //!
    void disconnectFromServer(const Message& m);

  } // namespace comms
} // namespace stateline
