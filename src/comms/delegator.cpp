//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"

#include "comms/datatypes.hpp"
#include "comms/endpoint.hpp"
#include "comms/protocol.hpp"
#include "comms/router.hpp"

#include <string>

#include <easylogging/easylogging++.h>
#include <numeric>

namespace stateline { namespace comms {

Delegator::PendingBatch::PendingBatch(std::string address, std::vector<double> data, std::size_t numJobTypes)
  : address{std::move(address)}
  , data{std::move(data)}
  , results(numJobTypes) // Pre-allocate the vector
  , numJobsDone{0}
{
}

Delegator::State::State(zmq::context_t& ctx, const DelegatorSettings& settings)
  : requester{ctx, zmq::socket_type::router, "toRequester"}
  , network{ctx, zmq::socket_type::router, "toNetwork"}
  , settings{settings}
{
}

void Delegator::State::addWorker(const std::string& address, const std::pair<JobType, JobType>& jobTypesRange)
{
  Worker w{address, jobTypesRange};

  // TODO: why not create lazily?
  for (auto i = jobTypesRange.first; i <= jobTypesRange.second; ++i)
    w.times.emplace(std::piecewise_construct,
        std::forward_as_tuple(i),
        std::forward_as_tuple(0.1)); // TODO make this a setting

  workers.emplace(address, w);

  LOG(INFO)<< "Worker " << address << " connected, supporting jobtypes: "; // TODO
}

void Delegator::State::addBatch(const std::string& address, JobID id, std::vector<double> data)
{
  // Add the batch as a pending request.
  auto ret = pending.emplace(std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(address, std::move(data), settings.numJobTypes));

  // Add each job in the batch to the queue.
  for (JobType i = 0; i < settings.numJobTypes; i++)
  {
    jobQueue.emplace_back(ret.first, i + 1);
  }

  VLOG(2) << pending.size() << " requests pending";
}

struct Delegator::RequesterEndpoint : Endpoint<RequesterEndpoint>
{
  State& delegator;

  RequesterEndpoint(State& delegator)
    : Endpoint<RequesterEndpoint>{delegator.requester}
    , delegator{delegator}
  {
  }

  void onBatchJob(const Message& m)
  {
    auto batchJob = protocol::unserialise<protocol::BatchJob>(m.data);
    VLOG(2) << "Received batch";

    delegator.addBatch(m.address, batchJob.id, std::move(batchJob.data));
  }
};

struct Delegator::NetworkEndpoint : Endpoint<NetworkEndpoint>
{
  State& delegator;
  JobID lastJobID;

  NetworkEndpoint(State& delegator)
    : Endpoint<NetworkEndpoint>{delegator.network}
    , delegator{delegator}
    , lastJobID{0}
  {
  }

  void onHello(const Message& m)
  {
    const auto hello = protocol::unserialise<protocol::Hello>(m.data);
    delegator.addWorker(m.address, hello.jobTypesRange);

    // Use the more lenient timeout as the agreed timeout
    const auto timeout = std::max(std::chrono::seconds{hello.hbTimeoutSecs},
        delegator.settings.heartbeatTimeout);
    delegator.network.startHeartbeats(m.address, timeout);

    // Send a WELCOME message to the worker
    protocol::Welcome welcome;
    welcome.hbTimeoutSecs = timeout.count();

    delegator.network.send({m.address, WELCOME, serialise(welcome)});
  }

  void idle()
  {
    for (auto it = delegator.jobQueue.begin(); it != delegator.jobQueue.end(); )
    {
      // TODO: find the best worker
      Worker *worker = delegator.workers.size() == 0 ? nullptr : &delegator.workers.begin()->second;
      if (!worker)
      {
        ++it;
        continue;
      }

      protocol::Job job;
      job.id = ++lastJobID;
      job.data = it->batch->second.data;

      delegator.network.send({worker->address, JOB, serialise(job)});

      it->startTime = Clock::now();
      worker->inProgress.emplace(job.id, std::move(*it));

      // TODO: investigate lazy removal
      it = delegator.jobQueue.erase(it);
    }
  }

  void onResult(const Message& m)
  {
    // TODO: refactor into State
    const auto it = delegator.workers.find(m.address);
    if (it == delegator.workers.end())
      return;

    const auto result = protocol::unserialise<protocol::Result>(m.data);
    Worker& worker = it->second;
    Job& job = worker.inProgress.at(result.id);
    PendingBatch& batch = job.batch->second;

    worker.times.at(job.type).add(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - job.startTime).count());

    batch.results[job.type - 1] = result.data; // job types are 1-indexed
    batch.numJobsDone++;

    if (batch.numJobsDone == batch.results.size())
    {
      protocol::BatchResult batchResult;
      batchResult.id = job.batch->first;
      batchResult.data = std::move(batch.results);

      delegator.requester.send({batch.address, BATCH_RESULT, serialise(batchResult)});
      delegator.pending.erase(job.batch);
    }

    // Remove job from work in progress store
    worker.inProgress.erase(result.id);
  }

  /*
  void onBye(const Message& m) { hb.disconnect(m.address); }

  void onAny(const Message& m) { hb.update(m.address); }
};

  // TODO: could we compute these lazily? i.e. each time a new job is assigned,
  // we update the expected finishing time?
  uint usTillDone(const Delegator::Worker& w, uint jobType)
  {
    uint t = 0;
    for (auto const& i : w.workInProgress)
      t += timeForJob(w, i.second.type);
    //now add the expected time for the new job
    t += timeForJob(w, jobType);
    return t;
  }*/
};

Delegator::Delegator(zmq::context_t& ctx, const DelegatorSettings& settings)
    : state_{ctx, settings}
{
  // Initialise the local sockets
  state_.requester.bind(settings.requesterAddress);
  state_.network.bind(settings.networkAddress);

  // TODO: network_.setFallback([&](const Message& m) { sendFailed(m); });

  LOG(INFO) << "Delegator listening on " << settings.networkAddress;
}

void Delegator::poll()
{
  bool running = false; // poll for one iteration
  start(running);
}

void Delegator::start(bool& running)
{
  RequesterEndpoint requester{state_};
  NetworkEndpoint network{state_};

  Router<RequesterEndpoint, NetworkEndpoint> router{"delegator", std::tie(requester, network)};

  const auto onIdle = [&network]()
  {
    network.idle();
    // TODO: Ugly.
    network.socket().heartbeats().idle();
  };

  do
  {
    router.poll(onIdle);
  } while (running);
}

/*
Delegator::Worker* Delegator::bestWorker(uint jobType, uint maxJobs)
{
  // TODO: can we do this using an STL algorithm?
  Delegator::Worker* best = nullptr;
  uint bestTime = std::numeric_limits<uint>::max();
  for (auto& w : workers_)
  {
    if (jobType >= w.second.jobTypesRange.first &&
        jobType <  w.second.jobTypesRange.second &&
        w.second.workInProgress.size() < maxJobs)
    {
      uint t = usTillDone(w.second, jobType);
      if (t < bestTime)
      {
        best = &w.second;
        bestTime = t;
      }
    }
  }
  return best;
}

void Delegator::disconnectWorker(const std::string& addr)
{
  const auto it = workers_.find(m.address);
  if (it == workers_.end())
    return;

  LOG(INFO)<< "Worker " << addr << " disconnected: re-assigning their jobs";

  // Push all the unfinished work back into the queue
  for (const auto& kv : it->wip)
    jobQueue_.push_front(kv.second);

  workers_.erase(it);
}

void Delegator::sendFailed(const Message& m)
{
  LOG(INFO) << "Failed to send to " << m.address;

  disconnectWorker(m.address);
} */

} }
