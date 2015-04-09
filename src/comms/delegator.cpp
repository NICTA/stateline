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
      auto fDisconnect = [&](const Message& m) { disconnectWorker(m); };
      auto fNewWorker = [&](const Message &m)
      {
        //forwarding to HEARTBEAT
        heartbeat_.send(m);
        std::string worker = m.address.front();
        if (workers_.count(worker) == 0)
          connectWorker(m);
        // Polite to reply
        network_.send({{worker}, HELLO});
      };

      auto fRcvRequest = [&](const Message &m) { receiveRequest(m); };
      auto fRcvResult = [&](const Message &m) { receiveResult(m); };
      auto fForwardToHB = [&](const Message& m) { heartbeat_.send(m); };
      auto fForwardToNetwork = [&](const Message& m) { network_.send(m); };
      auto fForwardToHBAndDisconnect = [&](const Message& m)
        { fForwardToHB(m); fDisconnect(m); };

      // Bind functionality to the router
      const uint REQUESTER_SOCKET = 0, HB_SOCKET = 1, NETWORK_SOCKET = 2;

      router_.bind(REQUESTER_SOCKET, REQUEST, fRcvRequest);
      router_.bind(NETWORK_SOCKET, HELLO, fNewWorker);
      router_.bind(NETWORK_SOCKET, RESULT, fRcvResult);
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

    void Delegator::connectWorker(const Message& msg)
    {
      // Worker can now be 'connected'
      // add jobtypes
      std::vector<std::string> jobTypes; 
      boost::split(jobTypes, msg.data[0], boost::is_any_of(":"));
      Worker w {msg.address, jobTypes, {}}; 
      std::string id = w.address.front();
      workers_.insert(std::make_pair(id,w));
      LOG(INFO)<< " Worker " << id << " connected.";
    }

    std::string nextJobID()
    {
      static uint n=0;
      return std::to_string(n++);
    }

    void Delegator::receiveRequest(const Message& msg)
    {
      std::string id = boost::algorithm::join(msg.address, ":");
      std::vector<std::string> jobTypes; 
      boost::split(jobTypes, msg.data[0], boost::is_any_of(":"));
      Request r {msg.address, jobTypes, std::vector<std::string>(jobTypes.size()), 0};
      requests_.insert(std::make_pair(id, r));
      uint idx=0;
      for (auto const& t : jobTypes)
      {
        Job j = {t, nextJobID(), id, idx};
        jobQueue_.push_back(j);
        idx++;
      }
    }

    void Delegator::receiveResult(const Message& msg)
    {
      std::string worker = msg.address.front();
      std::string jobID = msg.data[0];
      Job& j = workers_[worker].workInProgress[jobID];
      
      // add receipts
      Request& r = requests_[j.requesterID];
      r.results[j.requesterIndex] = msg.data[1];
      r.nDone++;
      
      if (r.nDone == r.jobTypes.size())
      {
        requester_.send({r.address, REQUEST, r.results});
        requests_.erase(j.requesterID);
      }
      
      //remove job from work in progress store
      workers_[worker].workInProgress.erase(jobID);
    }

    void Delegator::onPoll()
    {
      //stand-in SUPER simple
      Job& j = jobQueue_.front();
      for (auto const& w : workers_)
      {
        if (w.second.jobTypes.count(j.type))
        {
          network_.send(Message({w.second.address, JOB, j.data}));
          w.second.workInProgress.insert(std::make_pair(j.id, j));
          jobQueue.pop_front();
          break;
        }
      }
    }

    void Delegator::disconnectWorker(const Message& goodbyeFromWorker)
    {
      //first address
      std::string worker = goodbyeFromWorker.address.front();

      Worker& w = workers_[worker];
      for (auto const& j : w.workInProgress)
        jobQueue_.push_front(j);
      workers_.erase(worker);
    }

    void Delegator::sendFailed(const Message& msgToWorker)
    {
      LOG(INFO)<< "Failed to send to " << msgToWorker.address.front();
      // messages to and from a worker both had that workers address at the
      // front so disconnect will work in both cases
      disconnectWorker(msgToWorker);
    }

  } // namespace stateline
} // namespace comms
