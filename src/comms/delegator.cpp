//!
//! \file comms/delegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
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
    Delegator::Delegator(const std::string& commonSpecData, 
        const std::map<JobID, std::string>& jobSpecData,
        const DelegatorSettings& settings)
        : msNetworkPoll_(settings.msPollRate),
          commonSpecData_(commonSpecData),
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

      // Map external job IDs to internal consecutive IDs
      JobID internalJobId = 0;
      for (auto &jobSpec : jobSpecData)
      {
        // Map the JobID given in the spec to an internal ID
        jobIdMap_[jobSpec.first] = internalJobId;

        // Copy the corresponding spec into our internal spec vector which uses
        // contiguous ID values.
        jobSpecData_.push_back(jobSpec.second);
        internalJobId++;
      }

      VLOG(3) << "Attaching functionality to router";
      // // Specify the Delegator functionality
      auto fInit = std::bind(&Delegator::sendWorkerProblemSpec, this, ph::_1);
      auto fConnect = std::bind(&Delegator::connectWorker, this, ph::_1);
      auto fNewJob = std::bind(&Delegator::newJob, this, ph::_1);

      auto fDisconnect = std::bind(&Delegator::disconnectWorker, this, ph::_1);
      auto fSwapJobs = std::bind(&Delegator::jobSwap, this, ph::_1);
      auto fSendFailed = std::bind(&Delegator::sendFailed, this, ph::_1);

      auto fForwardToHB = [&] (const Message&m)
      {
        this->router_.send(SocketID::HEARTBEAT, m);};
      auto fForwardToNetwork = [this] (const Message&m)
      {
        this->router_.send(SocketID::NETWORK,m);};

      // // Bind these functions to the router
      router_(SocketID::NETWORK).onRcvHELLO.connect(fInit);
      router_(SocketID::NETWORK).onRcvHELLO.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvJOBREQUEST.connect(fConnect);
      router_(SocketID::REQUESTER).onRcvJOB.connect(fNewJob);
      router_(SocketID::NETWORK).onRcvJOBSWAP.connect(fSwapJobs);
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
      delete context_;
      delete heartbeat_;
    }

    void Delegator::sendWorkerProblemSpec(const Message& msgHelloFromWorker)
    {
      // we're not actully going to add the worker to the 'connected'
      // list until it comes back with a jobrequest! has to solve the
      // problemspec first...
      //most recently appended address
      LOG(INFO)<< "Initialising worker " << msgHelloFromWorker.address.back();

      // Get the job IDs that this worker offers to do
      std::vector<JobID> jobs;
      detail::unserialise<std::uint32_t>(msgHelloFromWorker.data[0], jobs);

      std::vector<std::string> repData;
      repData.push_back(commonSpecData_);

      for (auto job : jobs)
      {
        VLOG(1) << "Worker offered to solve job with ID " << job;

        if (jobIdMap_.count(job))
        {
          VLOG(1) << "\t and we have a spec for it! " << job;
          repData.push_back(std::to_string(job));
          repData.push_back(jobSpecData_[jobIdMap_[job]]);
        }
        else
        {
          // Send back an empty string as the spec
          repData.push_back(std::to_string(job));
          repData.push_back("");
        }
      }

      //Send back problemspec
      router_.send(SocketID::NETWORK,
          Message(msgHelloFromWorker.address, PROBLEMSPEC, repData));
    }

    void Delegator::connectWorker(const Message& msgJobRequestFromMinion)
    {
      // Worker has just completed the problemspec so can now be 'connected'
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

      std::deque<Message>& queue = jobQueues_[id];
      if (!queue.empty())
      {
        std::string worker = msgRequestFromMinion.address.back();
        //send a job from the job queue
        Message r = queue.front();
        // keep where the job came from, add new destination
        for (auto const& a : msgRequestFromMinion.address)
        {
          r.address.push_back(a);
        }
        router_.send(SocketID::NETWORK, r);
        workerToJobMap_[worker].push_back(queue.front());
        queue.pop_front();
      }
      else
      {
        // Add the minion to the request queue
        requestQueues_[id].push_back(msgRequestFromMinion.address);
      }
    }

    void Delegator::newJob(const Message& msgJobFromRequester)
    {
      uint id = detail::unserialise<std::uint32_t>(msgJobFromRequester.data[0]);

      std::deque<std::vector<std::string>>& queue = requestQueues_[id];

      //forward straight to minion if there's one waiting
      if (!queue.empty())
      {
        Message r = msgJobFromRequester;
        // append the address of this minion
        for (auto const& a : queue.front())
        {
          r.address.push_back(a);
        }
        // send the job
        router_.send(SocketID::NETWORK, r);
        // add to WIP list
        std::string worker = r.address.back();
        workerToJobMap_[worker].push_back(msgJobFromRequester);
        // Remove the minion from the request queue 
        queue.pop_front();
      }
      else
      {
        jobQueues_[id].push_back(msgJobFromRequester);
      }
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
      sendJob(Message( { minion, worker }, JOBSWAP, { id }));
    }

    std::vector<JobID> Delegator::jobs() const
    {
      std::vector<JobID> jobs;
      for (auto job : jobIdMap_)
        jobs.push_back(job.first);
      return jobs;
    }

  } // namespace stateline
} // namespace comms
