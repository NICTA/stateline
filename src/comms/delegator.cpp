//!
//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"

#include <string>

#include "comms/datatypes.hpp"
#include "comms/serial.hpp"

namespace stateline
{
  namespace comms
  {
    Delegator::Delegator(const std::vector<JobType> &jobTypes,
        const DelegatorSettings& settings)
        : msNetworkPoll_(settings.msPollRate),
          running_(true)
    {
      namespace ph = std::placeholders;

      context_ = new zmq::context_t(1);
      heartbeat_ = new ServerHeartbeat(*context_, settings.heartbeat);

      std::unique_ptr<zmq::socket_t> requester(new zmq::socket_t(*context_, ZMQ_ROUTER));
      std::unique_ptr<zmq::socket_t> heartbeat(new zmq::socket_t(*context_, ZMQ_PAIR));
      std::unique_ptr<zmq::socket_t> network(new zmq::socket_t(*context_, ZMQ_ROUTER));

      // Initialise the local sockets
      requester->bind(DELEGATOR_SOCKET_ADDR.c_str());
      heartbeat->bind(SERVER_HB_SOCKET_ADDR.c_str());

      // Initialise the network socket
      std::string address = "tcp://*:" + std::to_string(settings.port);
      network->bind(address.c_str());
      LOG(INFO)<< "Delegator listening on " << address;

      // Pass the sockets to the router
      router_.add_socket(SocketID::REQUESTER, requester);
      router_.add_socket(SocketID::HEARTBEAT, heartbeat);
      router_.add_socket(SocketID::NETWORK, network);

      VLOG(3) << "Attaching functionality to router";

      // Specify the Delegator functionality
      auto fNewJob = std::bind(&Delegator::newJob, this, ph::_1);
      auto fDisconnect = std::bind(&Delegator::disconnectWorker, this, ph::_1);
      auto fSwapJobs = [&](const Message &m)
      {
        // TODO: refactor this by merging connectWorker and jobSwap
        std::string worker = m.address.back();
        if (workerToJobMap_.count(worker) == 0)
          connectWorker(m);
        else
          jobSwap(m);
      };

      auto fSendFailed = std::bind(&Delegator::sendFailed, this, ph::_1);

      auto fForwardToHB = [this] (const Message&m) { this->router_.send(SocketID::HEARTBEAT, m); };
      auto fForwardToNetwork = [this] (const Message&m) { this->router_.send(SocketID::NETWORK,m); };

      // Bind these functions to the router
      router_(SocketID::REQUESTER).onRcvWORK.connect(fNewJob);
      router_(SocketID::NETWORK).onRcvHELLO.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvWORK.connect(fSwapJobs);
      router_(SocketID::NETWORK).onFailedSend.connect(fSendFailed);
      router_(SocketID::NETWORK).onRcvHEARTBEAT.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvGOODBYE.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvGOODBYE.connect(fDisconnect);
      router_(SocketID::HEARTBEAT).onRcvHEARTBEAT.connect(fForwardToNetwork);
      router_(SocketID::HEARTBEAT).onRcvGOODBYE.connect(fDisconnect);

      VLOG(3) << "Functionality assignment complete";

      VLOG(2) << "Starting the Routers";
      router_.start(msNetworkPoll_, running_);
    }

    Delegator::~Delegator()
    {
      running_ = false;
      VLOG(1) << "Deleting context";
      delete context_;
      VLOG(1) << "Context deleted";
      VLOG(1) << "Deleting heartbeat";
      delete heartbeat_;
      VLOG(1) << "heartbeat deleted";
    }

    void Delegator::connectWorker(const Message& msgJobRequestFromMinion)
    {
      // Worker can now be 'connected'
      std::string worker = msgJobRequestFromMinion.address.back();
      std::string minion = msgJobRequestFromMinion.address.front();
      VLOG(1) << "minion " << worker << ":" << minion << " ready";

      if (workerToJobMap_.count(worker) == 0)
      {
        workerToJobMap_.insert(std::make_pair(worker, std::vector<Message>()));
      }

      LOG(INFO)<< workerToJobMap_.size() << " Workers currently connected.";

      // this message actually came from a minion, so make sure we send a job
      // to them
      sendJob(msgJobRequestFromMinion);
    }

    void Delegator::sendJob(const Message& msgRequestFromMinion)
    {
      uint id = detail::unserialise<std::uint32_t>(msgRequestFromMinion.data[0]);

      PendingMinion minion;

      // Find the first job that can be given to the minion.
      for (auto it = pendingJobs_.begin(); it != pendingJobs_.end(); ++it)
      {
        if (minion->canDo(it))
        {
          // Append the minion's address to the job and forward it to the network.
          Message r = it->message;

          // Append new address to the minion.
          for (const auto &a : msgRequestFromMinion.address)
            r.address.push_back(a);

          router_.send(SocketID::NETWORK, r);
          pendingJobs_.remove(it); // TODO: we can just label it as removed
          return;
        }
      }

      // This minion can't do any job yet, add it to the pending minion queue.
      pendingMinions_.push(msgRequestFromMinion);
    }

    void Delegator::newJob(Message msgJobFromRequester)
    {
      uint id = detail::unserialise<std::uint32_t>(msgJobFromRequester.data[0]);

      PendingJob job;

      // Find the first minion that can do this new job.
      for (auto it = pendingMinions_.begin(); it != pendingMinions_.end(); ++it)
      {
        if (it->canDo(job))
        {
          // Append the minion's address to the message and forward it to the network.
          for (const auto &a : it->address)
            msgJobFromRequester.address.push_back(a);

          router_.send(SocketID::NETWORK, msgJobFromRequester);
          pendingMinions_.remove(it); // TODO: we can just label it as removed
          return;
        }
      }

      // No minions can do this job, add it to the pending job queue
      pendingJobs_.push_back(job);
    }

    void Delegator::disconnectWorker(const Message& goodbyeFromWorker)
    {
      //most recently appended address
      std::string worker = goodbyeFromWorker.address.back();

      //remove the worker's minions from all the request queues
      uint id = 0;
      for (auto& queue : requestQueues_)
      {
        auto requestPred = [&](const std::vector<std::string>& s)
        {
          bool match = s.back() == worker;
          if (match)
          VLOG(1) << "Removing " << addressAsString(s) << " from request queue " << id;
          return match;
        };
        auto wjBegin = std::remove_if(queue.second.begin(), queue.second.end(), requestPred);
        auto wjEnd = queue.second.end();
        queue.second.erase(wjBegin, wjEnd);
        id++;
      }

      //remove all the worker's jobs from work in progress queue 
      //and push them back onto the (appropriate) job queue
      for (auto const& j : workerToJobMap_[worker])
      {
        uint id = detail::unserialise<std::uint32_t>(j.data[0]);
        VLOG(1) << "Requeueing " << j << "onto queue " << id;
        jobQueues_[id].push_front(j);
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
      std::string id = r.data.back();
      r.data.pop_back();
      //remove job from work in progress queue
      auto remPred = [&](const Message& s)
      {
        bool match = r.address == s.address;
        if (match)
        VLOG(3) << "removing " << s << " from WIP queue";
        return match;
      };
      auto remBegin = std::remove_if(workerToJobMap_[worker].begin(), workerToJobMap_[worker].end(), remPred);
      auto remEnd = workerToJobMap_[worker].end();
      workerToJobMap_[worker].erase(remBegin, remEnd);
      // send it on to the requester
      router_.send(SocketID::REQUESTER, r);
      // give the minion a new job
      sendJob(Message( { minion, worker }, WORK, { id }));
    }

  } // namespace stateline
} // namespace comms
