//!
//! The delegator is the intermediary between requesters and workers. It acts
//! as the server to which workers connect, and requesters submit jobs.
//!
//! \file comms/delegator.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <set>
#include <string>
#include <deque>
// Prerequisites
#include <glog/logging.h>
#include <zmq.hpp>
// Project
#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/serverheartbeat.hpp"

//! Address that the requesters connect their sockets to.
const std::string DELEGATOR_SOCKET_ADDR = "inproc://delegator";

namespace stateline
{
  namespace comms
  {
    //! Requester object that takes jobs and returns results. Communicates with
    //! a delegator living in a (possibly) different thread.
    //!
    class Delegator
    {
    public:
      //! Create a new delegator.
      //!
      //! \param commonSpecData The serialised common problem specification.
      //! \param jobId The available job IDs.
      //! \param jobSpecData The serialised problem specifications for each job.
      //! \param jobResultsData The serialised problem results for each job.
      //! \param settings The configuration object.
      //!
      Delegator(const std::string& commonSpecData, const std::vector<uint>& jobId, const std::vector<std::string>& jobSpecData,
                const std::vector<std::string>& jobResultsData, const DelegatorSettings& settings);

      //! Safely stops all polling threads and cleans up.
      //!
      ~Delegator()
      {
        stop();
      }

      //! Return a reference to the context object owned by the delegator.
      //! this allows a requester to use inproc sockets and connect.
      //!
      //! \return a reference to the zmq::context_t object
      //!
      zmq::context_t& zmqContext()
      {
        return context_;
      }

      //! Start the delegator socket polling.
      //!
      void start()
      {
        router_.start(msNetworkPoll_);
        heartbeat_.start();
      }

      //! Stop the delegator socket polling.
      //!
      void stop()
      {
        heartbeat_.stop();
        router_.stop();
      }

      //! Initialise a worker by giving it the problem specification.
      //! 
      //! \param m The HELLO message the new worker sent.
      //!
      void sendWorkerProblemSpec(const Message& m);

      //! Connect a worker that has previously been sent a problem spec.
      //!
      //! \param m The JOBREQUEST message the connecting worker sent.
      //!
      void connectWorker(const Message& m);

      //! Send a job to a worker that has requested one.
      //! 
      //! \param m the job request message.
      //!
      void sendJob(const Message& m);

      //! Disconnect a worker by removing it from the list of connected workers.
      //! 
      //! \param m The GOODBYE message the connecting worker sent.
      //!
      void disconnectWorker(const Message& m);

      //! When a send fails, we disconnect the worker.
      //!
      //! \param m The message that failed to send.
      //!
      void sendFailed(const Message& m);

      //! Process a job request by sending a job to the worker
      //! if available, otherwise add to a needs job queue.
      //!
      //! \param m The JOBREQUEST message.
      //!
      void jobRequest(const Message& m);

      //! Get a result from a worker, then swap it for a new job.
      //! 
      //! \param m The JOBSWAP message.
      //!
      void jobSwap(const Message& m);

      //! Receive a new job from the requesters, and add it to the queue or
      //! send it directly to an idle worker.
      //! 
      //! \param m The JOBSWAP message.
      //!
      void newJob(const Message& m);

    private:
      // Polling times
      int msNetworkPoll_;
      // Sockets
      zmq::context_t context_;
      SocketRouter router_;
      // Cached for fast sending to each client
      std::string commonSpecData_;
      std::vector<uint> jobId_;
      std::vector<std::string> jobSpecData_;
      std::vector<std::string> jobResultsData_;
      // Fault tolerance support
      std::map<std::string, std::vector<Message>> workerToJobMap_;
      // The queues for jobs
      std::vector<std::deque<Message>> jobQueues_;
      std::vector<std::deque<std::vector<std::string>>>requestQueues_;
    std::map<uint, uint> jobIdMap_;
    // Heartbeating System
    ServerHeartbeat heartbeat_;
  };

}
 // namespace comms
}// namespace obsidian
