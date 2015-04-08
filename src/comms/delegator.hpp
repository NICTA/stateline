//!
//! The delegator is the intermediary between requesters and workers. It acts
//! as the server to which workers connect, and requesters submit jobs.
//!
//! \file comms/delegator.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <set>
#include <string>
#include <deque>

#include <glog/logging.h>
#include <zmq.hpp>

#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/serverheartbeat.hpp"

namespace stateline
{
  namespace comms
  {
    //! Address that the requesters connect their sockets to.
    // const std::string DELEGATOR_SOCKET_ADDR = "inproc://delegator";
    const std::string DELEGATOR_SOCKET_ADDR = "ipc:///tmp/sl_delegator.socket";

    //! Requester object that takes jobs and returns results. Communicates with
    //! a delegator living in a (possibly) different thread.
    //!
    class Delegator
    {
      public:
        //! Create a new delegator.
        //!
        //! \param settings The configuration object.
        //!
        Delegator(zmq::context_t& context, const DelegatorSettings& settings);

        // Delegators can't be copied.
        Delegator(const Delegator &other) = delete;

        //! Safely stops all polling threads and cleans up.
        //!
        ~Delegator();

        void start();

        // //! Return a reference to the context object owned by the delegator.
        // //! this allows a requester to use inproc sockets and connect.
        // //!
        // //! \return a reference to the zmq::context_t object
        // //!
        // zmq::context_t& zmqContext()
        // {
        //   return *context_;
        // }

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
        struct PendingJob
        {
          std::string type;
          Message job;
        };

        struct PendingMinion
        {
          std::string type;
          Address address;
        };

        zmq::context_t& context_; 

        // Sockets
        Socket requester_;
        Socket heartbeat_;
        Socket network_;
        SocketRouter router_;

        std::deque<PendingJob> pendingJobs_;
        std::deque<PendingMinion> pendingMinions_;
        std::map<std::string, std::vector<Message>> workerToJobMap_;

        uint msPollRate_;
        HeartbeatSettings hbSettings_;

        bool running_;
    };

  } // namespace comms
}// namespace stateline
