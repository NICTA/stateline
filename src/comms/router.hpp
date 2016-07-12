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
  //! Create a new router.
  //!
  Router(std::string name, const std::tuple<Endpoints&...>& endpoints)
    : name_{std::move(name)}
    , endpoints_{endpoints}
  {
    pollList_.reserve(sizeof...(Endpoints));
    sockets_.reserve(sizeof...(Endpoints));

    meta::apply_all(endpoints, [&pollList = pollList_, &sockets = sockets_](auto& e)
    {
      // Need to cast a socket_t into (void *)
      pollList.push_back({(void *)e.socket().zmqSocket(), 0, ZMQ_POLLIN, 0});
      sockets.push_back(static_cast<SocketBase*>(&e.socket()));
    });
  }

  template <class IdleCallback>
  void poll(const IdleCallback& callback)
  {
    // Figure out how long we can wait for before any SocketBase times out
    const auto it = std::min_element(sockets_.begin(), sockets_.end(),
        detail::compSocketTimeouts);

    if (it != sockets_.end())
    {
      if ((*it)->heartbeats().hasTimeout())
      {
        const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(
            (*it)->heartbeats().nextTimeout() - Heartbeat::Clock::now());

        zmq::poll(pollList_, std::max(wait, std::chrono::milliseconds{0}));
      }
      else
      {
        // Poll indefinitely
        zmq::poll(pollList_);
      }
    }

    // Handle each poll event
    for (std::size_t i = 0; i < pollList_.size(); i++)
    {
      bool newMsg = pollList_[i].revents & ZMQ_POLLIN;
      if (newMsg)
      {
        LOG(INFO) << "Router " << name_ << " received new message from endpoint "
          << meta::apply<std::string>(endpoints_, i, [](auto& m) { return m.socket().name(); });

        meta::apply<void>(endpoints_, i, [](auto& e) { return e.accept(); });
      }
    }

    callback();
  }

private:
  std::string name_;
  std::tuple<Endpoints&...> endpoints_;
  std::vector<SocketBase*> sockets_; // TODO: make this into array
  std::vector<zmq::pollitem_t> pollList_;
};

} }
