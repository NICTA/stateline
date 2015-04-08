//!
//! \file comms/router.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
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
    uint index(uint socketIndex, const Subject& s)
    {
      return (uint)Subject::Size * socketIndex + (uint)s;
    }

    SocketRouter::SocketRouter(const std::string& name, const std::vector<Socket*>& sockets)
      : name_(name),
        sockets_(sockets)
    {
      for (auto& s : sockets_)
      {
        s->setLinger(0);
        pollList_.push_back({s, 0, ZMQ_POLLIN, 0});
      }
    }

    SocketRouter::~SocketRouter()
    {
      // VLOG(1) << "Router " << name_ << " waiting for polling thread to return";
      // if (threadReturned_.valid())
      // {
      //   threadReturned_.wait();
      // }
      // VLOG(1) << "Waiting complete";
    }

    void SocketRouter::bind(uint socketIndex, const Subject& s, const Callback& f)
    {
      callbacks_[index(socketIndex, s)] = f;
    }

    void SocketRouter::bindOnPoll(const std::function<void(void)>& f)
    {
      onPoll_ = f;
    }

    // void SocketRouter::start(int msPerPoll, bool& running)
    // {
    // }

    // SocketHandler& SocketRouter::operator()(const SocketID& id)
    // {
    //   return *handlers_[indexMap_.left.at(id)];
    // }

    // void SocketRouter::send(const SocketID& id, const Message& msg)
    // {
    //   std::vector<std::unique_ptr<zmq::socket_t>>* sockets = &sockets_;
    //   if (threadSockets_) // the poll thread now owns the sockets
    //     sockets = threadSockets_;

    //   uint index = indexMap_.left.at(id);
    //   if (msg.subject != stateline::comms::HEARTBEAT)
    //     VLOG(3) << "Router " << name_ <<  " sending " << msg << " to " << id;
    //   else
    //     VLOG(4) << "Router " << name_ << " sending " << msg << " to " << id;

    //   try
    //   {
    //     stateline::comms::send(*((*sockets)[index]), msg);
    //   } catch (zmq::error_t const& e)
    //   {
    //     // No route to host?
    //     if (e.num() == EHOSTUNREACH)
    //     {
    //       handlers_[index]->onFailedSend(msg);
    //     } else
    //     {
    //       LOG(ERROR)<< "ROUTER " << name_ <<  " COULD NOT SEND A MESSAGE\n";
    //     }
    //   }
    // }

    // Message SocketRouter::receive(const SocketID& id)
    // {
    //   std::vector<std::unique_ptr<zmq::socket_t>>* sockets = &sockets_;
    //   if (threadSockets_) // the poll thread now owns the sockets
    //     sockets = threadSockets_;
    //   uint index = indexMap_.left.at(id);
    //   Message m =  comms::receive(*((*sockets)[index]));
    //   VLOG(3) << "Router " << name_ << " received " << m << " from " << id;
    //   return m;
    // }

    // this is an int because -1 indicates no timeout
    void SocketRouter::poll(int msWait, bool& running)
    {
      while (running)
      {
        // block until a message arrives
        zmq::poll(&(pollList_[0]), pollList_.size(), msWait);
        // figure out which socket it's from
        for (uint i = 0; i < pollList_.size(); i++)
        {
          bool newMsg = pollList_[i].revents & ZMQ_POLLIN;
          if (newMsg)
          {
            Message msg = sockets_[i]->receive();
            callbacks_[index(i, msg.subject)](msg);
            // receive(*(sockets[i]), *(handlers_[i]), index);
          }
        }
        onPoll_();
      }
      LOG(INFO) << "Router " << name_ << "'s Poll thread has exited loop, must be shutting down";
    }

    // void SocketRouter::receive(zmq::socket_t& socket, SocketHandler& h, const SocketID& idx)
    // {
    //   auto msg = stateline::comms::receive(socket);
    //   if (msg.subject != stateline::comms::HEARTBEAT)
    //     VLOG(3) << "Router " << name_ << " received " << msg << " from " << idx;
    //   else
    //     VLOG(4) << "Router " << name_ << " received " << msg << " from " << idx;

    //   switch (msg.subject)
    //   {
    //     case stateline::comms::HELLO:
    //       h.onRcvHELLO(msg);
    //       break;
    //     case stateline::comms::HEARTBEAT:
    //       h.onRcvHEARTBEAT(msg);
    //       break;
    //     case stateline::comms::WORK:
    //       h.onRcvWORK(msg);
    //       break;
    //     case stateline::comms::GOODBYE:
    //       h.onRcvGOODBYE(msg);
    //       break;
    //   }
    // }

  } // namespace comms
} // namespace stateline
