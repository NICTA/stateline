//! The delegator is the intermediary between requesters and workers. It acts
//! as the server to which workers connect, and requesters submit jobs.
//!
//! \file comms/delegator.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "settings.hpp"
#include "heartbeat.hpp"
#include "common/circularbuffer.hpp"

#include <chrono>
#include <set>
#include <string>
#include <list>
#include <atomic>

namespace stateline { namespace comms {

//! Address that the requesters connect their sockets to.
// const std::string DELEGATOR_SOCKET_ADDR = "inproc://delegator";
const std::string DELEGATOR_SOCKET_ADDR = "ipc:///tmp/sl_delegator.socket";

//! Responsible for allocating jobs to workers.
//!
class Delegator
{
public:
  //! Construct a new delegator.
  //!
  //! \param ctx ZMQ context.
  //! \param settings The configuration object.
  //!
  Delegator(zmq::context_t& ctx, const DelegatorSettings& settings);

  Delegator(const Delegator&) = delete;
  Delegator& operator=(const Delegator&) = delete;

  void start(bool& running);

private:
  using std::chrono::high_resolution_clock = hrc;

  struct Request
  {
    std::string address;
    std::vector<double> data;
    std::vector<double> results;
    int nDone;

    Request(std::string address, std::vector<double> data, std::size_t numJobTypes)
      : address{std::move(address)}
      , data{std::move(data)}
      , results(numJobTypes) // Pre-allocate the vector
      , nDone{0}
    {
    }
  };

  struct Job
  {
    JobType type;
    JobID id;
    BatchID batchID;
    hrc::time_point startTime;

    Request(JobID id, JobType type,
  };

  struct Worker
  {
    std::string address;
    std::pair<uint, uint> jobTypesRange; //! 

    //! 
    std::map<JobID, Job> workInProgress;
    std::map<JobType, CircularBuffer<uint>> times;
    hrc::time_point lastResultTime;

    Worker(std::vector<std::string> address,
           std::pair<uint, uint> jobTypesRange)
      : address{std::move(address)}
      , jobTypesRange{std::move(jobTypesRange)}
      , lastResultTime{hrc::now()}
    {
    }
  };

  void idle();

  // TODO: does this need to be a member function?
  Worker* bestWorker(uint jobType, uint maxJobs);

  // Sockets
  Socket requester_;
  Socket worker_;

  std::map<std::string, Worker> workers_;
  std::map<std::string, Request> requests_;
  std::list<Job> jobQueue_;

  JobID nextJobId_;
};

} }
