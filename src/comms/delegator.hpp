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

#include "comms/datatypes.hpp"
#include "comms/socket.hpp"
#include "comms/utils.hpp"
#include "settings.hpp"

#include <chrono>
#include <map>
#include <string>
#include <list>
#include <atomic>

namespace stateline { namespace comms {

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

  void poll();

  void start(bool& running);

private:
  using Clock = std::chrono::high_resolution_clock;

  struct PendingBatch
  {
    std::string address; // need to know who to send this back to
    std::vector<double> data; // the batch job data
    std::vector<double> results; // table of pending results
    int numJobsDone; // number of completed jobs so far

    PendingBatch(std::string address, std::vector<double> data, std::size_t numJobTypes);
  };

  using PendingBatchContainer = std::map<BatchID, PendingBatch>;

  struct Job
  {
    PendingBatchContainer::iterator batch;
    JobType type;
    Clock::time_point startTime;

    Job(PendingBatchContainer::iterator batch, JobType type)
      : batch(batch)
      , type(type)
    {
    }
  };

  struct Worker
  {
    std::string address;
    std::pair<JobType, JobType> jobTypesRange;
    std::map<JobID, Job> inProgress;
    std::map<JobType, ExpMovingAverage<float>> times;

    Worker(std::string address,
           const std::pair<JobType, JobType>& jobTypesRange)
      : address{std::move(address)}
      , jobTypesRange{jobTypesRange}
    {
    }
  };

  /*
  void idle();

  // TODO: does this need to be a member function?
  Worker* bestWorker(uint jobType, uint maxJobs);*/

  struct State
  {
    Socket requester;
    Socket network;
    DelegatorSettings settings;
    std::map<std::string, Worker> workers;
    PendingBatchContainer pending;
    std::list<Job> jobQueue;

    State(zmq::context_t& ctx, const DelegatorSettings& settings);

    void addWorker(const std::string& address, const std::pair<JobType, JobType>& jobTypeRange);

    void addBatch(const std::string& address, JobID id, std::vector<double> data);
  };

  struct RequesterEndpoint;
  struct NetworkEndpoint;

  State state_;
  /*
  std::map<std::string, Request> requests_;
  std::list<Job> jobQueue_;

  JobID nextJobId_;*/
};

} }
