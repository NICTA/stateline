//!
//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"

#include <string>
#include <boost/algorithm/string.hpp>

#include "comms/datatypes.hpp"
#include "comms/thread.hpp"

namespace stateline
{
  namespace comms
  {
    Delegator::Delegator(zmq::context_t& context, const DelegatorSettings& settings, bool& running)
        : context_(context),
          requester_(context, ZMQ_ROUTER, "toRequester"),
          heartbeat_(context, ZMQ_PAIR, "toHBRouter"),
          network_(context, ZMQ_ROUTER, "toNetwork"),
          router_("main", {&requester_, &heartbeat_, &network_}),
          msPollRate_(settings.msPollRate),
          hbSettings_(settings.heartbeat),
          running_(running)
    {
      // Initialise the local sockets
      requester_.bind(DELEGATOR_SOCKET_ADDR);
      heartbeat_.bind(SERVER_HB_SOCKET_ADDR);
      network_.setFallback([&](const Message& m) { sendFailed(m); });
      std::string address = "tcp://*:" + std::to_string(settings.port);
      network_.bind(address);

      LOG(INFO) << "Delegator listening on tcp://*:" + std::to_string(settings.port);

      // Specify the Delegator functionality
      auto fNewJob = [&](const Message& m) { newJob(m); };
      auto fDisconnect = [&](const Message& m) { disconnectWorker(m); };
      auto fNewWorker = [&](const Message &m)
      {
        //forwarding to HEARTBEAT
        heartbeat_.send(m);
        std::string worker = m.address.back();
        if (workerToJobMap_.count(worker) == 0)
          connectWorker(m);
        // Polite to reply
        network_.send({{worker}, HELLO});
      };

      auto fSwapJobs = [&](const Message &m) { jobSwap(m); };
      auto fForwardToHB = [&](const Message& m) { heartbeat_.send(m); };
      auto fForwardToNetwork = [&](const Message& m) { network_.send(m); };
      auto fForwardToHBAndDisconnect = [&](const Message& m)
        { fForwardToHB(m); fDisconnect(m); };

      // Bind functionality to the router
      const uint REQUESTER_SOCKET = 0, HB_SOCKET = 1, NETWORK_SOCKET = 2;

      router_.bind(REQUESTER_SOCKET, WORK, fNewJob);
      router_.bind(NETWORK_SOCKET, HELLO, fNewWorker);
      router_.bind(NETWORK_SOCKET, WORK, fSwapJobs);
      router_.bind(NETWORK_SOCKET, HEARTBEAT, fForwardToHB);
      router_.bind(NETWORK_SOCKET, GOODBYE, fForwardToHBAndDisconnect);
      router_.bind(HB_SOCKET, HEARTBEAT, fForwardToNetwork);
      router_.bind(HB_SOCKET, GOODBYE, fDisconnect);
    }

    Delegator::~Delegator()
    {
    }

    void Delegator::start()
    {
      // Start the heartbeat thread and router
      auto future = startInThread<ServerHeartbeat>(std::ref(running_), std::ref(context_), std::cref(hbSettings_));
      router_.poll(msPollRate_, running_);
      future.wait();
    }

    void Delegator::connectWorker(const Message& msgHelloWorker)
    {
      // Worker can now be 'connected'
      std::string worker = msgHelloWorker.address.back();
      VLOG(1) << "worker " << worker <<  " ready";
      if (workerToJobMap_.count(worker) == 0)
      {
        workerToJobMap_.insert(std::make_pair(worker, std::vector<Message>()));
      }
      LOG(INFO)<< workerToJobMap_.size() << " Workers currently connected.";
    }

    void Delegator::sendJob(const Message& msgRequestFromMinion)
    {
      std::set<std::string> jobTypes;
      boost::split(jobTypes, msgRequestFromMinion.data[0], boost::is_any_of(":"));

      PendingMinion minion{jobTypes, msgRequestFromMinion.address};

      VLOG(3) << "Received new work request from minion";
      // Find the first job that can be given to the minion.
      for (auto it = pendingJobs_.begin(); it != pendingJobs_.end(); ++it)
      {
        if (jobTypes.count(it->type))
        {
          // Append the minion's address to the job and forward it to the network.
          Message r = it->job;

          // Append new address to the minion.
          for (const auto &a : msgRequestFromMinion.address)
            r.address.push_back(a);

          VLOG(3) << "Found existing job for minion, sending...";
          network_.send(r);
          //Add job to WIP vector for that worker
          std::string worker = r.address.back();
          workerToJobMap_[worker].push_back(it->job);
          VLOG(3) << "Worker now has " << workerToJobMap_[worker].size() << " jobs in progress";
          pendingJobs_.erase(it); // TODO: we can just label it as removed
          return;
        }
      }

      VLOG(3) << "No work for minion, pushing to pending queue";
      // This minion can't do any job yet, add it to the pending minion queue.
      pendingMinions_.push_back(minion);
    }

    void Delegator::newJob(const Message& msgJobFromRequester)
    {
      std::string jobType = msgJobFromRequester.data[0];

      PendingJob job {jobType, msgJobFromRequester};

      VLOG(3) << "New job received.";
      // Find the first minion that can do this new job.
      for (auto it = pendingMinions_.begin(); it != pendingMinions_.end(); ++it)
      {
        if (it->types.count(jobType))
        {
          // Append the minion's address to the message and forward it to the network.
          VLOG(3) << "Found a minion to do the work, sending on.";
          Message r = Message(msgJobFromRequester);
          for (const auto &a : it->address)
            r.address.push_back(a);

          network_.send(r);
          //Add job to WIP vector for that worker
          std::string worker = it->address.back();
          workerToJobMap_[worker].push_back(msgJobFromRequester);
          VLOG(3) << "Worker now has " << workerToJobMap_[worker].size() << " jobs in progress";
          pendingMinions_.erase(it); // TODO: we can just label it as removed
          return;
        }
      }

      VLOG(3) << "Couldn't find a minion to complete, adding to job queue.";
      // No minions can do this job, add it to the pending job queue
      pendingJobs_.push_back(job);
    }

    void Delegator::disconnectWorker(const Message& goodbyeFromWorker)
    {
      //most recently appended address
      std::string worker = goodbyeFromWorker.address.back();

      //remove the worker's minions from all the request queues
      auto requestPred = [&](const PendingMinion& s)
      {
        bool match = s.address.back() == worker;
        if (match)
        VLOG(3) << "Removing " << addressAsString(s.address) << " from request queue.";
        return match;
      };
      auto wjBegin = std::remove_if(pendingMinions_.begin(), pendingMinions_.end(), requestPred);
      auto wjEnd = pendingMinions_.end();
      pendingMinions_.erase(wjBegin, wjEnd);

      //remove all the worker's jobs from work in progress queue 
      //and push them back onto the (appropriate) job queue
      for (auto const& j : workerToJobMap_[worker])
      {
        std::string jobType = j.data[0];
        VLOG(3) << "Requeueing " << j << "onto queue";
        pendingJobs_.push_front({jobType, j});
      }

      // disconnect the worker
      VLOG(1) << "Disconnecting " << worker;
      workerToJobMap_.erase(worker);

      LOG(INFO)<< workerToJobMap_.size() << " Workers currently connected.";
    }

    void Delegator::sendFailed(const Message& msgToWorker)
    {
      LOG(INFO)<< "Failed to send to " << msgToWorker.address.back();
      // messages to and from a worker both had that workers address at the
      // front so disconnect will work in both cases
      disconnectWorker(msgToWorker);
    }

    void Delegator::jobSwap(const Message& msgResultFromMinion)
    {
      Message r = msgResultFromMinion;
      // Remove and store worker address
      std::string worker = r.address.back();
      r.address.pop_back();
      // Remove and store minion address
      std::string minion = r.address.back();
      r.address.pop_back();
      // Remove and store requested id for new job
      std::string newJobType = r.data.back();
      r.data.pop_back();
      //remove job from work in progress queue
      auto remPred = [&](const Message& s)
      {
        bool match = true;
        // we only match the back set of addresses because we don't care about
        // who did the job, only who sent it.
        for (uint i=0; i<r.address.size();i++)
        {
          match = match && (r.address[i] == s.address[i]);
        }
        if (match)
          VLOG(3) << "Job found-- removing from WIP queue";
        return match;
      };

      auto remBegin = std::remove_if(workerToJobMap_[worker].begin(), workerToJobMap_[worker].end(), remPred);
      auto remEnd = workerToJobMap_[worker].end();
      workerToJobMap_[worker].erase(remBegin, remEnd);
      // send it on to the requester
      requester_.send(r);
      // give the minion a new job
      sendJob(Message( { minion, worker }, WORK, { newJobType }));
    }

  } // namespace stateline
} // namespace comms
