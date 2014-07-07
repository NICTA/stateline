//!
//! \file comms/router.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <chrono>
#include <cstdio>
#include "comms/router.hpp"
#include <thread>
#include <glog/logging.h>

typedef std::chrono::high_resolution_clock hrc;

namespace stateline
{
  namespace comms
  {
    std::ostream& operator<<(std::ostream& os, const SocketID& id)
    {
      static std::map<SocketID, std::string> m( { { SocketID::REQUESTER, "REQUESTER" }, { SocketID::MINION, "MINION" }, { SocketID::WORKER,
                                                                                                                          "WORKER" },
                                                  { SocketID::NETWORK, "NETWORK" }, { SocketID::HEARTBEAT, "HEARTBEAT" }, { SocketID::ALPHA,
                                                                                                                            "ALPHA" },
                                                  { SocketID::BETA, "BETA" } });
      os << m[id];
      return os;
    }

    SocketRouter::SocketRouter()
        : running_(false)
    {
    }

    SocketRouter::~SocketRouter()
    {
      SocketRouter::stop();
    }

    void SocketRouter::add_socket(SocketID idx, std::unique_ptr<zmq::socket_t>& socket)
    {
      uint i = sockets_.size();
      indexMap_.insert(typename IndexBiMap::value_type(idx, i));
      sockets_.push_back(std::move(socket));
      handlers_.push_back(std::unique_ptr < SocketHandler > (new SocketHandler()));
      pollList_.push_back( { *(sockets_[i]), 0, ZMQ_POLLIN, 0 });
    }

    void SocketRouter::start(int msPerPoll)
    {
      LOG(INFO)<< "starting router with timeout at " << msPerPoll << " ms";
      running_ = true;
      // poll with timeout
      threadReturned_ = std::async(std::launch::async, &SocketRouter::poll, this, msPerPoll);
    }

    SocketHandler& SocketRouter::operator()(const SocketID& id)
    {
      return *handlers_[indexMap_.left.at(id)];
    }

    void SocketRouter::stop()
    {
      if (running_)
      {
        running_ = false;
        threadReturned_.wait();
      }
    }

    void SocketRouter::send(const SocketID& id, const Message& msg)
    {
      uint index = indexMap_.left.at(id);
      if (msg.subject != stateline::comms::HEARTBEAT)
        VLOG(3) << "Sending " << msg << " to " << id;
      else
        VLOG(4) << "Sending " << msg << " to " << id;

      try
      {
        stateline::comms::send(*(sockets_[index]), msg);
      } catch (zmq::error_t const& e)
      {
        // No route to host?
        if (e.num() == EHOSTUNREACH)
        {
          handlers_[index]->onFailedSend(msg);
        } else
        {
          LOG(ERROR)<< "COULD SEND A MESSAGE\n";
        }
      }
    }

    Message SocketRouter::receive(const SocketID& id)
    {
      uint index = indexMap_.left.at(id);
      Message m(HELLO);
      try
      {
        m = stateline::comms::receive(*(sockets_[index]));
      } catch (zmq::error_t const& e)
      {
        LOG(ERROR)<< "COULD NOT RECEIVE A MESSAGE\n";
      }
      return m;
    }

    // this is an int because -1 indicates no timeout
    bool SocketRouter::poll(int msWait)
    {
      while (running_)
      {
        // block until a message arrives
        zmq::poll(&(pollList_[0]), pollList_.size(), msWait);
        // figure out which socket it's from
        for (uint i = 0; i < pollList_.size(); i++)
        {
          bool newMsg = pollList_[i].revents & ZMQ_POLLIN;
          if (newMsg)
          {
            SocketID index = indexMap_.right.at(i);
            receive(*(sockets_[i]), *(handlers_[i]), index);
          }
          handlers_[i]->onPoll();
        }
      }
      return true;
    }

    void SocketRouter::receive(zmq::socket_t& socket, SocketHandler& h, const SocketID& idx)
    {
      if (!running_)
      {
        return;
      }

      auto msg = stateline::comms::receive(socket);
      if (msg.subject != stateline::comms::HEARTBEAT)
        VLOG(3) << "Received " << msg << " from " << idx;
      else
        VLOG(4) << "Received " << msg << " from " << idx;
      switch (msg.subject)
      {
      case stateline::comms::HELLO:
        h.onRcvHELLO(msg);
        break;
      case stateline::comms::HEARTBEAT:
        h.onRcvHEARTBEAT(msg);
        break;
      case stateline::comms::PROBLEMSPEC:
        h.onRcvPROBLEMSPEC(msg);
        break;
      case stateline::comms::JOBREQUEST:
        h.onRcvJOBREQUEST(msg);
        break;
      case stateline::comms::JOB:
        h.onRcvJOB(msg);
        break;
      case stateline::comms::JOBSWAP:
        h.onRcvJOBSWAP(msg);
        break;
      case stateline::comms::ALLDONE:
        h.onRcvALLDONE(msg);
        break;
      case stateline::comms::GOODBYE:
        h.onRcvGOODBYE(msg);
        break;
      }
    }

  } // namespace comms
} // namespace obsidian
