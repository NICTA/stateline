//!
//! The worker object connects with a delegator, then forwards job requests
//! to a set of minions that actually perform the work. These minions then
//! return the worker results, which are forwarded back to the delegator.
//!
//! \file comms/worker.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <string>

// Prerequisites
#include <glog/logging.h>
#include <zmq.hpp>

// Project
#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/datatypes.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/clientheartbeat.hpp"

//! Address that the minions connect their sockets to.
const std::string WORKER_SOCKET_ADDR = "inproc://worker";

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
      //! Build a new worker that can handle multiple types of jobs.
      //!
      //! \param jobIDs A list of job IDs that the worker can do.
      //! \param settings The configuration object.
      //!
      Worker(const std::vector<uint>& jobIDs, const WorkerSettings& settings);
 
      // Workers can't be copied.
      Worker(const Worker &other) = delete;

      //! Destructor. Safely stops all polling threads and cleans up.
      //!
      ~Worker()
      {
        stop();
      }

      //! Stop the worker polling sockets
      //!
      void stop()
      {
        heartbeat_.stop();
        router_.stop();
      }

      //! Return a ref to the context object owned by the worker.
      //! This allows a minion to use inproc sockets and connect.
      //!
      //! \return A reference to the zmq::context_t object
      //!
      zmq::context_t& zmqContext()
      {
        return context_;
      }

      //! Return a reference to the problemspec so that the minions can
      //! instantiate their sockets.
      //!
      //! \return A reference to the worker-owned problemspec.
      //!
      const std::string& globalSpec()
      {
        return globalSpec_;
      }

      //! Return the individual job specifications for the minions.
      //!
      //! \return A reference to the worker-owned jobspec.
      //!
      const std::string& jobSpec(uint jobID)
      {
        return jobSpecs_[jobID];
      }

      //! Return a set of job IDs that are enabled.
      //!
      //! \return Set of jobs IDs that are enabled.
      //!
      const std::set<uint> jobsEnabled() const
      {
        return jobsEnabled_;
      }

      //! Return the result of a particular job.
      //!
      //! \param jobID The ID of the job.
      //! \return A const reference to the serialised result of a job.
      //!
      const std::string& jobResults(uint jobID)
      {
        return jobResults_[jobID];
      }

    private:
      // Context for all local sockets
      zmq::context_t context_;
      SocketRouter router_;
      std::string globalSpec_;
      std::set<uint> jobsEnabled_;
      std::map<uint, std::string> jobSpecs_;
      std::map<uint, std::string> jobResults_;

      // Heartbeating System
      ClientHeartbeat heartbeat_;
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
} // namespace obsidian
