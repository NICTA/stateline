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
      switch(id)
      {
        case SocketID::REQUESTER: os << "REQUESTER"; break;
        case SocketID::MINION: os << "MINION"; break;
        case SocketID::WORKER: os << "WORKER"; break;
        case SocketID::NETWORK: os << "NETWORK"; break;
        case SocketID::HEARTBEAT: os << "HEARTBEAT"; break;
        case SocketID::ALPHA: os << "ALPHA"; break;
        case SocketID::BETA: os << "BETA"; break;
        default: os << "UNKNOWN";
      }

      return os;
    }

    SocketRouter::SocketRouter()
      : threadSockets_(nullptr)
    {
    }

    SocketRouter::~SocketRouter()
    {
      VLOG(1) << "Waiting for polling thread to return";
      threadReturned_.wait();
      VLOG(1) << "Waiting complete";
    }

    void SocketRouter::add_socket(SocketID idx, std::unique_ptr<zmq::socket_t>& socket)
    {
      int lingerTime = 0;
      socket->setsockopt(ZMQ_LINGER, &lingerTime, sizeof(int));

      uint i = sockets_.size();
      indexMap_.insert(typename IndexBiMap::value_type(idx, i));
      sockets_.push_back(std::move(socket));
      handlers_.push_back(std::unique_ptr < SocketHandler > (new SocketHandler()));
    }

    void SocketRouter::start(int msPerPoll, bool& running)
    {
      LOG(INFO)<< "starting router with timeout at " << msPerPoll << " ms";
      // poll with timeout
      threadReturned_ = std::async(std::launch::async, &SocketRouter::poll, this, msPerPoll, std::ref(running));
    }

    SocketHandler& SocketRouter::operator()(const SocketID& id)
    {
      return *handlers_[indexMap_.left.at(id)];
    }

    void SocketRouter::send(const SocketID& id, const Message& msg)
    {
      std::vector<std::unique_ptr<zmq::socket_t>>* sockets = &sockets_;
      if (threadSockets_) // the poll thread now owns the sockets
        sockets = threadSockets_;

      uint index = indexMap_.left.at(id);
      if (msg.subject != stateline::comms::HEARTBEAT)
        VLOG(3) << "Sending " << msg << " to " << id;
      else
        VLOG(4) << "Sending " << msg << " to " << id;

      try
      {
        stateline::comms::send(*((*sockets)[index]), msg);
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
      std::vector<std::unique_ptr<zmq::socket_t>>* sockets = &sockets_;
      if (threadSockets_) // the poll thread now owns the sockets
        sockets = threadSockets_;
      uint index = indexMap_.left.at(id);
      return comms::receive(*((*sockets)[index]));
    }

    // this is an int because -1 indicates no timeout
    void SocketRouter::poll(int msWait, bool& running)
    {
      // Move the sockets to local control
      std::vector<std::unique_ptr<zmq::socket_t>> sockets;
      std::vector<zmq::pollitem_t> pollList;
      for (uint i=0; i< sockets_.size();i++)
      {
        sockets.push_back(std::unique_ptr<zmq::socket_t>(std::move(sockets_[i])));
        pollList.push_back( { *(sockets[i]), 0, ZMQ_POLLIN, 0 });
      }
      // Sockets now stored in here
      threadSockets_ = &sockets;
      // Empty the old socket list
      sockets_.clear();
      
      while (running)
      {
        // block until a message arrives
        zmq::poll(&(pollList[0]), pollList.size(), msWait);
        // figure out which socket it's from
        for (uint i = 0; i < pollList.size(); i++)
        {
          bool newMsg = pollList[i].revents & ZMQ_POLLIN;
          if (newMsg)
          {
            SocketID index = indexMap_.right.at(i);
            receive(*(sockets[i]), *(handlers_[i]), index);
          }
          handlers_[i]->onPoll();
        }
      }
      LOG(INFO) << "Poll thread has exited loop, must be shutting down";
    }

    void SocketRouter::receive(zmq::socket_t& socket, SocketHandler& h, const SocketID& idx)
    {
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
} // namespace stateline
