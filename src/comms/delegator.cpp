//!
//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"

#include "comms/datatypes.hpp"
#include "comms/endpoint.hpp"
#include "comms/protobuf.hpp"
#include "comms/thread.hpp"

#include <string>

#include <easylogging/easylogging++.h>
#include <numeric>

namespace stateline { namespace comms {

namespace {

struct RequesterEndpoint : Endpoint<RequesterEndpoint, Socket>
{
  Socket& worker;

  RequesterEndpoint(Socket& requester, Socket& worker)
    : Endpoint<RequesterEndpoint>(requester)
    , worker(worker)
  {
  }

  void onBatchJob(const Message& m)
  {
    const auto batchJob = stringToProtobuf<messages::BatchJob>(m.data);

    VLOG(2) << "Received batch with " << batch.job_types_size() << " jobs";

    // Add the batch as a pending request.
    requests_.emplace(std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(m.address, batchJob.data(), jobTypes.size()));

    // Add each job in the batch to the queue.
    for (std::size_t i = 0; i < batchJob.job_type_size(); i++)
    {
      jobQueue_.emplace_back(nextJobId_++, batchJob.job_type(i));
    }

    VLOG(2) << requests_.size() << " requests pending";
  }
};

struct WorkerEndpoint : Endpoint<WorkerEndpoint, Socket>
{
  Socket& requester;

  WorkerEndpoint(Socket& worker, Socket& requester)
    : Endpoint<WorkerEndpoint>{worker}
    , requester{requester}
  {
  }

  void onHello(const Message& m)
  {
    // add jobtypes
    // TODO: job types

    const auto hello = stringToProtobuf<messages::Hello>(m.data);

    Worker w{m.address, jobTypeRange};
    for (auto i = jobTypeRange.first; i < jobTypeRange.second; ++i)
      w.times.emplace(i, CircularBuffer<uint>{10});

    workers_.emplace(m.address, w);

    LOG(INFO)<< "Worker " << id << " connected, supporting jobtypes: "; // TODO

    hb.connect(m.address, std::chrono::seconds{10}); // TODO
  }

  void onResult(const Message& m)
  {
    const auto it = workers_.find(m.address);
    if (it == workers_.end())
      return;

    const auto result = protobufToString<messages::Result>(m.data);
    const Job& job = it->second.wip.at(result.id());

    // timing information
    const auto now = hrc::now();

    // estimate time the worker spent on this job
    const auto elapsedTime = now - job.startTime;

    const auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count();
    it->times.at(j.type).push_back(usecs);
    it->lastResultTime = now;

    Request& req = requests_[j.batchID];
    req.results[j.jobTypeIndex] = result.data();
    req.numCompleted++;

    if (req.numCompleted == req.results.size())
    {
      messages::BatchResult batchResult;
      batchResult.set_id(req.id);

      for (const auto& r : req.results)
        batchResult.add_data(r);

      requester_.send({req.address, BATCH_RESULT, protobufToString(batchResult)});
      requests_.erase(job.batchID);
    }

    // Remove job from work in progress store
    worker.wip.erase(job.id);
  }

  void onBye(const Message& m) { hb.disconnect(m.address); }

  void onAny(const Message& m) { hb.update(m.address); }
};

  // TODO: should this be a float?
  uint timeForJob(const Delegator::Worker& w, uint jobType)
  {
    auto& times = w.times.at(jobType);
    uint totalTime = std::accumulate(std::begin(times), std::end(times), 0);
    uint avg;
    if (times.size() > 0)
      avg = totalTime / times.size();
    else
      avg = 5; // very short initial guess to encourage testing
    return avg;
  }

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
  }

}

Delegator::Delegator(zmq::context_t& ctx, const DelegatorSettings& settings)
    : requester_{ctx, ZMQ_ROUTER, "toRequester"}
    , network_{ctx, ZMQ_ROUTER, "toNetwork"}
    , msPollRate_{settings.msPollRate}
    , hbSettings_{settings.heartbeat}
    , nextJobId_{1}
{
  // Initialise the local sockets
  requester_.bind(DELEGATOR_SOCKET_ADDR);
  network_.connect(settings);

  network_.setFallback([&](const Message& m) { sendFailed(m); });

  network_.bind("tcp://*:" + std::to_string(settings.port));

  LOG(INFO) << "Delegator listening on tcp://*:" << settings.port;
}

void Delegator::start(bool& running)
{
  while (running)
  {
    poll(std::tie(requester_, worker_), pollWait_);

    idle();
    hb_.idle();
  }
}

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

void Delegator::idle()
{
  const uint maxJobs = 10;
  auto i = std::begin(jobQueue_);
  while (i != std::end(jobQueue_))
  {
    auto worker = bestWorker(i->type, maxJobs);
    if (!worker)
    {
      ++i;
      continue;
    }

    std::string& data = requests_[i->requesterID].data;
    network_.send({worker->address, JOB, {std::to_string(i->type), i->id, data}});
    i->startTime = std::chrono::high_resolution_clock::now();
    worker->workInProgress.insert(std::make_pair(i->id, *i));

    // TODO: could we avoid removing this? What if we just mark it as being removed,
    // and ignore it when we're polling? We can reap these 'zombie' jobs whenever
    // they get removed from the front of the queue (assuming there's no job starvation).
    // This way, we can keep using a queue instead of a list, so we can get
    // better performance through cache locality.
    jobQueue_.erase(i++);
  }
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
}

} // namespace stateline

} // namespace comms
