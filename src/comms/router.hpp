//!
//! A router that implements polling on multiple zeromq sockets,
//! and a signal-based callback system depending on the socket and the subject
//! of the message. Implementing STATELINEP-SW (STATELINE Server-Wrapper Protocol)
//!
//! \file comms/router.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "common/meta.hpp"
#include "comms/message.hpp"
#include "comms/socket.hpp"

#include <array>
#include <chrono>
#include <future>
#include <easylogging/easylogging++.h>

namespace stateline { namespace comms {

namespace detail {

inline bool compSocketTimeouts(const SocketBase* a, const SocketBase* b)
{
  if (!a->heartbeats().hasTimeout()) return false;
  if (!b->heartbeats().hasTimeout()) return true;
  return a->heartbeats().nextTimeout() < b->heartbeats().nextTimeout();
}

}

//! Implements a reactor pattern dispatcher. Use this class to attach event handlers
//! to sockets.
//!
template <class... Endpoints>
class Router
{
public:
  static_assert(sizeof...(Endpoints) > 0, "Must have at least one endpoint");

  //! Create a new router.
  //!
  Router(std::string name, const std::tuple<Endpoints&...>& endpoints)
    : name_{std::move(name)}
    , endpoints_{endpoints}
    , sockets_{meta::mapAll(endpoints, [](auto& e) { return static_cast<SocketBase*>(&e.socket()); })}
    , pollList_{meta::mapAll(endpoints, [](auto &e) { return zmq::pollitem_t{(void *)e.socket().zmqSocket(), 0, ZMQ_POLLIN, 0}; })}
  {
  }

  int pollWaitTime()
  {
    const auto it = std::min_element(sockets_.begin(), sockets_.end(),
        detail::compSocketTimeouts);

    if ((*it)->heartbeats().hasTimeout())
    {
      const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(
          (*it)->heartbeats().nextTimeout() - Heartbeat::Clock::now());

      return std::max(wait, std::chrono::milliseconds{0}).count(); // 0 for return immediately if we have to heartbeat
    }
    else
    {
      // Poll indefinitely
      return -1;
    }
  }

  template <class IdleCallback>
  void poll(const IdleCallback& callback)
  {
    zmq::poll(pollList_.data(), pollList_.size(), pollWaitTime());

    // Handle each poll event
    meta::enumerateAll(endpoints_, [&pollList = pollList_](const auto i, auto& endpoint)
    {
      bool newMsg = pollList[i].revents & ZMQ_POLLIN;
      if (newMsg)
      {
        //TODO LOG(INFO) << "Router " << name_ << " received new message from endpoint "
          //<< endpoint.socket().name();

        endpoint.accept();
      }
    });

    callback();
  }

private:
  std::string name_;
  std::tuple<Endpoints&...> endpoints_;
  std::array<SocketBase*, sizeof...(Endpoints)> sockets_;
  std::array<zmq::pollitem_t, sizeof...(Endpoints)> pollList_;
};

} }
