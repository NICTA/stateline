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
    Delegator::Delegator(const DelegatorSettings& settings)
        : msNetworkPoll_(settings.msPollRate),
          router_("main"),
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
      auto fNewWorker = [&] (const Message &m)
      {
        //forwarding to HEARTBEAT
        router_.send(SocketID::HEARTBEAT, m);
        std::string worker = m.address.back();
        if (workerToJobMap_.count(worker) == 0)
          connectWorker(m);
        // Polite to reply
        router_.send(SocketID::NETWORK, Message({worker}, HELLO));
      };
      
      auto fSwapJobs = [&](const Message &m)
      {
        jobSwap(m);
      };

      auto fSendFailed = std::bind(&Delegator::sendFailed, this, ph::_1);

      auto fForwardToHB = [this] (const Message&m) 
      { 
        router_.send(SocketID::HEARTBEAT, m);
      };

      auto fForwardToNetwork = [this] (const Message&m) { this->router_.send(SocketID::NETWORK,m); };

      // Bind these functions to the router
      router_(SocketID::REQUESTER).onRcvWORK.connect(fNewJob);
      router_(SocketID::NETWORK).onRcvHELLO.connect(fNewWorker);
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
      std::string jobType = msgRequestFromMinion.data[0];

      PendingMinion minion {jobType, msgRequestFromMinion.address};

      VLOG(1) << "Received new work request from minion";
      // Find the first job that can be given to the minion.
      for (auto it = pendingJobs_.begin(); it != pendingJobs_.end(); ++it)
      {
        if (it->type == jobType)
        {
          // Append the minion's address to the job and forward it to the network.
          Message r = it->job;

          // Append new address to the minion.
          for (const auto &a : msgRequestFromMinion.address)
            r.address.push_back(a);

          VLOG(1) << "Found existing job for minion, sending...";
          router_.send(SocketID::NETWORK, r);
          //Add job to WIP vector for that worker
          std::string worker = r.address.back();
          workerToJobMap_[worker].push_back(it->job);
          VLOG(1) << "Worker now has " << workerToJobMap_[worker].size() << " jobs in progress";
          pendingJobs_.erase(it); // TODO: we can just label it as removed
          return;
        }
      }

      VLOG(1) << "No work for minion, pushing to pending queue";
      // This minion can't do any job yet, add it to the pending minion queue.
      pendingMinions_.push_back(minion);
    }

    void Delegator::newJob(const Message& msgJobFromRequester)
    {
      std::string jobType = msgJobFromRequester.data[0];

      PendingJob job {jobType, msgJobFromRequester};

      VLOG(1) << "New job received.";
      // Find the first minion that can do this new job.
      for (auto it = pendingMinions_.begin(); it != pendingMinions_.end(); ++it)
      {
        if (it->type == jobType)
        {
          // Append the minion's address to the message and forward it to the network.
          VLOG(1) << "Found a minion to do the work, sending on.";
          Message r = Message(msgJobFromRequester);
          for (const auto &a : it->address)
            r.address.push_back(a);

          router_.send(SocketID::NETWORK, r); 
          //Add job to WIP vector for that worker
          std::string worker = it->address.back();
          workerToJobMap_[worker].push_back(msgJobFromRequester);
          VLOG(1) << "Worker now has " << workerToJobMap_[worker].size() << " jobs in progress";
          pendingMinions_.erase(it); // TODO: we can just label it as removed
          return;
        }
      }

      VLOG(1) << "Couldn't find a minion to complete, adding to job queue.";
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
        VLOG(1) << "Removing " << addressAsString(s.address) << " from request queue.";
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
        VLOG(1) << "Requeueing " << j << "onto queue";
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
        VLOG(3) << "Matching return work with WIP queue: " << r << " vs " << s;
        bool match = true;
        // we only match the back set of addresses because we don't care about
        // who did the job, only who sent it.
        for (uint i=0; i<r.address.size();i++)
        {
          VLOG(3) << "substring:" << r.address[i] << " vs " << s.address[i];
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
      router_.send(SocketID::REQUESTER, r);
      // give the minion a new job
      sendJob(Message( { minion, worker }, WORK, { newJobType }));
    }

  } // namespace stateline
} // namespace comms
