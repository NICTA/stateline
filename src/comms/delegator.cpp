//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"

#include "common/logging.hpp"
#include "comms/datatypes.hpp"
#include "comms/endpoint.hpp"
#include "comms/protocol.hpp"
#include "comms/router.hpp"

#include <string>

#include <numeric>

namespace stateline { namespace comms {

Delegator::Worker::JobDuration Delegator::Worker::estimatedFinishDurationForNewJob(JobType type) const
{
  const auto wipTime = std::accumulate(inProgress.begin(), inProgress.end(), 0.0,
      [this](const auto& a, const auto& b)
      {
        return a + times.at(b.second.type).average();
      });

  return JobDuration{static_cast<JobDuration::rep>(wipTime + times.at(type).average())};
}

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

void Delegator::State::addWorker(const std::string& address, std::pair<JobType, JobType> jobTypesRange)
{
  // 0 is a special job type indicating a wildcard
  if (jobTypesRange.first == 0)
    jobTypesRange.first = 1;

  if (jobTypesRange.second == 0)
    jobTypesRange.second = settings.numJobTypes;

  Worker w{address, jobTypesRange};

  // TODO: why not create lazily?
  for (auto i = jobTypesRange.first; i <= jobTypesRange.second; ++i)
    w.times.emplace(std::piecewise_construct,
        std::forward_as_tuple(i),
        std::forward_as_tuple(0.1)); // TODO make this a setting

  workers.emplace(address, w);

  SL_LOG(INFO)<< "New worker connected "
    << pprint("address", address, "jobTypes", jobTypesRange);
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

  SL_LOG(DEBUG) << pending.size() << " requests pending";
}

Delegator::Worker* Delegator::State::bestWorker(const JobType type)
{
  Delegator::Worker* bestWorker = nullptr;
  auto bestDuration = Worker::JobDuration::max();
  for (auto& it : workers)
  {
    auto& worker = it.second;
    if (type >= worker.jobTypesRange.first &&
        type <= worker.jobTypesRange.second &&
        worker.inProgress.size() < worker.maxJobs)
    {
      const auto duration = worker.estimatedFinishDurationForNewJob(type);
      if (duration < bestDuration ||
          (duration == bestDuration && worker.inProgress.size() < bestWorker->inProgress.size()))
      {
        bestWorker = &worker;
        bestDuration = duration;
      }
    }
  }
  return bestWorker;
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
      Worker *worker = delegator.bestWorker(it->type);
      if (!worker)
      {
        ++it;
        continue;
      }

      protocol::Job job;
      job.id = ++lastJobID;
      job.data = it->batch->second.data;

      forwardMessage(delegator.network, {worker->address, JOB, serialise(job)});

      it->startTime = Clock::now();
      worker->inProgress.emplace(job.id, std::move(*it));

      // TODO: investigate lazy removal
      it = delegator.jobQueue.erase(it);
    }

    // Call base class idle
    Endpoint<NetworkEndpoint>::idle();
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

    worker.times.at(job.type).add(std::chrono::duration<float, std::micro>(Clock::now() - job.startTime).count());

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

  void onBye(const Message& m)
  {
    socket().heartbeats().disconnect(m.address, DisconnectReason::USER_REQUESTED);
  }

  void onHeartbeatDisconnect(const std::string& addr, DisconnectReason)
  {
    auto it = delegator.workers.find(addr);
    if (it == delegator.workers.end())
      return;

    // Requeue the in progress jobs of this worker
    SL_LOG(INFO) << "Re-queuing " << it->second.inProgress.size() << " jobs from " << addr;
    for (const auto& kv : it->second.inProgress)
      delegator.jobQueue.emplace_front(kv.second);

    delegator.workers.erase(it);
  }
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

  const auto onIdle = [&network]() { network.idle(); };

  do
  {
    router.poll(onIdle);
  } while (running);
}

} }
