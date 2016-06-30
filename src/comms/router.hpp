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

namespace stateline
{

namespace comms
{

namespace detail
{

constexpr std::size_t index(std::size_t socketIndex, Subject s)
{
  return Subject::SIZE * socketIndex + static_cast<std::size_t>(s);
}

}

//! The callback function upon receipt of a message
using Callback = std::function<void(const Message& m)>;

//! Implements polling and configurable routing between an
//! arbitrary number of (pre-constructed) sockets. Functionality
//! is attached through a signal interface.
//!
template <class... Sockets>
class Router
{
public:
  //! Create a new socket router.
  //!
  Router(std::string name, const std::tuple<Sockets&...>& sockets)
    : name_{std::move(name)}
    , sockets_{sockets}
    , onPoll_{[]() { }}
  {
    pollList_.reserve(sizeof...(Sockets));

    meta::apply_all(sockets, [&list = pollList_](auto& s)
    {
      // Need to cast a socket_t into (void *)
      list.push_back({(void *)s.zmqSocket(), 0, ZMQ_POLLIN, 0});
    });
  }

  template <std::size_t SocketIndex, Subject s>
  void bind(const Callback& f)
  {
    std::get<detail::index(SocketIndex, s)>(callbacks_) = f;
  }

  template <class Callback>
  void bindOnPoll(const Callback& callback) { onPoll_ = callback; }

  void pollStep(std::chrono::milliseconds wait)
  {
    // Block until a message arrives or timeout
    zmq::poll(pollList_, wait);

    // Figure out which socket it's from
    for (uint i = 0; i < pollList_.size(); i++)
    {
      bool newMsg = pollList_[i].revents & ZMQ_POLLIN;
      if (newMsg)
      {
        auto msg = meta::apply<Message>(sockets_, i, [](auto& m) { return m.recv(); });

        VLOG(4) << "Router " << name_ << " received new message from socket "
          << meta::apply<std::string>(sockets_, i, [](auto& m) { return m.name(); }) << ": " << msg;
        callbacks_[detail::index(i, msg.subject)](std::move(msg));
      }
    }

    onPoll_();
  }

  //! Start the router polling with a polling loop frequency
  void poll(int msWait, bool& running)
  {
    while (running)
    {
      pollStep(std::chrono::milliseconds{msWait});
    }

    LOG(INFO) << "Router " << name_ << "'s Poll thread has exited loop, must be shutting down";
  }

private:
  std::string name_;
  std::tuple<Sockets&...> sockets_;
  std::vector<zmq::pollitem_t> pollList_;
  std::array<Callback, sizeof...(Sockets) * Subject::SIZE> callbacks_;
  std::function<void(void)> onPoll_;
};

} // namespace stateline

}// namespace comms
