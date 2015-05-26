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
#include <list>

#include <glog/logging.h>
#include <zmq.hpp>
#include <boost/circular_buffer.hpp>

#include "settings.hpp"
#include "messages.hpp"
#include "router.hpp"
#include "serverheartbeat.hpp"

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

    // TODO: move this to inside Delegator as private members


    class Delegator
    {
      public:
        //! Create a new delegator.
        //!
        //! \param settings The configuration object.
        //!
        Delegator(zmq::context_t& context, const DelegatorSettings& settings, bool& running);

        // Delegators can't be copied.
        Delegator(const Delegator &other) = delete;

        //! Safely stops all polling threads and cleans up.
        //!
        ~Delegator();

        void start();

      private:
        struct Request
        {
          std::vector<std::string> address;
          std::set<std::string> jobTypes;
          std::string data;
          std::vector<std::string> results;
          uint nDone;
        };

        struct Job
        {
          std::string type;
          std::string id;
          std::string requesterID;
          uint requesterIndex;
          std::chrono::high_resolution_clock::time_point startTime;
        };

        struct Result
        {
        };

        // TODO: public for now, some free functions in delegator.cpp need it
        // Either make those member functions or add them as friends
      public:
        struct Worker
        {
          std::vector<std::string> address;
          std::set<std::string> jobTypes;
          std::map<std::string, Job> workInProgress;
          std::map<std::string, boost::circular_buffer<uint>> times;
        };

      private:
        void onPoll();

        void receiveRequest(const Message& m);

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
        void receiveResult(const Message& m);

        // TODO: does this need to be a member function?
        Worker* bestWorker(const std::string& jobType, uint maxJobs);


        zmq::context_t& context_; 

        // Sockets
        Socket requester_;
        Socket heartbeat_;
        Socket network_;
        SocketRouter router_;

        std::map<std::string, Worker> workers_;
        std::map<std::string, Request> requests_;
        std::list<Job> jobQueue_;

        uint msPollRate_;
        HeartbeatSettings hbSettings_;

        bool& running_;
        uint nextJobId_;
    };

  } // namespace comms
}// namespace stateline
