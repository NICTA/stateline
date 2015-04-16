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
    namespace
    {
      std::string nextJobID()
      {
        static uint n=0;
        return std::to_string(n++);
      }

      // TODO: should this be a float?
      uint timeForJob(const Delegator::Worker& w, const std::string& jobType)
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
      uint usTillDone(const Delegator::Worker& w, const std::string& jobType)
      {
        uint t = 0;
        for (auto const& i : w.workInProgress)
          t += timeForJob(w, i.second.type);
        //now add the expected time for the new job
        t += timeForJob(w, jobType);
        return t;
      }

    }

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

      auto fOnPoll = [&] () {onPoll();};
      router_.bindOnPoll(fOnPoll);
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
      std::set<std::string> jobTypes; 
      boost::split(jobTypes, msg.data[0], boost::is_any_of(":"));

      Worker w {msg.address, jobTypes, {}, {}};
      for (auto const& s : jobTypes)
        w.times.insert(std::make_pair(s, boost::circular_buffer<uint>(10)));

      std::string id = w.address.front();
      workers_.insert(std::make_pair(id, w));
      LOG(INFO)<< " Worker " << id << " connected.";
    }

    void Delegator::receiveRequest(const Message& msg)
    {
      std::string id = boost::algorithm::join(msg.address, ":");
      std::set<std::string> jobTypes; 
      boost::split(jobTypes, msg.data[0], boost::is_any_of(":"));
      VLOG(2) << "New request Received, with " << jobTypes.size() << " jobs.";
      Request r {msg.address, jobTypes, msg.data[1], std::vector<std::string>(jobTypes.size()), 0};
      requests_.insert(std::make_pair(id, r));
      uint idx=0;
      for (auto const& t : jobTypes)
      {
        Job j = {t, nextJobID(), id, idx, {}}; //we're not starting with a job yet
        jobQueue_.push_back(j);
        idx++;
      }
      VLOG(2) << requests_.size() << " requests currently pending.";
    }

    void Delegator::receiveResult(const Message& msg)
    {
      std::string worker = msg.address.front();
      std::string jobID = msg.data[0];
      Job& j = workers_[worker].workInProgress[jobID];
      // timing information
      auto elapsedTime = std::chrono::high_resolution_clock::now() - j.startTime;
      uint usecs = std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count();
      workers_[worker].times[j.type].push_back(usecs);
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

    Delegator::Worker* Delegator::bestWorker(const std::string& jobType, uint maxJobs)
    {
      // TODO: can we do this using an STL algorithm?
      Delegator::Worker* best = nullptr;
      uint bestTime = std::numeric_limits<uint>::max();
      for (auto& w : workers_)
      {
        if (w.second.jobTypes.count(jobType) && w.second.workInProgress.size() < maxJobs)
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

    void Delegator::onPoll()
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
        network_.send({worker->address, JOB, {i->type, i->id, data}});
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

    void Delegator::disconnectWorker(const Message& goodbyeFromWorker)
    {
      //first address
      std::string worker = goodbyeFromWorker.address.front();

      Worker& w = workers_[worker];
      for (auto const& j : w.workInProgress)
        jobQueue_.push_front(j.second);
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
