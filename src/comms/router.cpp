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
        sockets_(sockets),
        callbacks_((uint)Subject::Size * sockets.size(), nullptr), // TODO: add default callback to throw exception
        onPoll_([](){})
    {
      for (auto s : sockets_)
      {
        s->setLinger(0);
        pollList_.push_back({s->socket_, 0, ZMQ_POLLIN, 0});
      }
    }

    SocketRouter::~SocketRouter()
    {
    }

    void SocketRouter::bind(uint socketIndex, const Subject& s, const Callback& f)
    {
      callbacks_[index(socketIndex, s)] = f;
    }

    void SocketRouter::bindOnPoll(const std::function<void(void)>& f)
    {
      onPoll_ = f;
    }

    // this is an int because -1 indicates no timeout
    void SocketRouter::poll(int msWait, bool& running)
    {
      VLOG(1) << "Router " << name_ << "'s poll thread has started";
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

            VLOG(4) << "Router " << name_ << " received new message from socket " << sockets_[i]->name() << ": " << msg;
            callbacks_[index(i, msg.subject)](msg);
          }
        }
        onPoll_();
      }
      LOG(INFO) << "Router " << name_ << "'s Poll thread has exited loop, must be shutting down";
    }

  } // namespace comms
} // namespace stateline
