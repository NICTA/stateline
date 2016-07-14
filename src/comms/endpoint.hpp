//! Provides a message handling layer above a socket.
//!
//! \file comms/endpoint.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/socket.hpp"
#include <iostream>

namespace stateline { namespace comms {

//! A wrapper around a socket that also has logic for handling different messages.
//! This class uses CRTP to statically dispatch callbacks based on message types.
//! To create a new endpoint, inherit from this class and implement any of the
//! callback handlers.
//!
template <class Base>
class Endpoint
{
public:
  explicit Endpoint(Socket& socket)
    : socket_{socket}
  {
  }

  Socket& socket() { return socket_; }

  void accept()
  {
    handle(socket_.recv());
  }

  void handle(const Message& m)
  {
    switch (m.subject)
    {
      case HEARTBEAT:
        self().onHeartbeat(m);
        break;

      case HELLO:
        self().onHello(m);
        break;

      case WELCOME:
        self().onWelcome(m);
        break;

      case JOB:
        self().onJob(m);
        break;

      case RESULT:
        self().onResult(m);
        break;

      default:
        // TODO: unrecognised subject
        onDefault(m);
        break;
    }

    onAny(m);
  }

  void onAny(const Message&) { }
  void onDefault(const Message&) { }
  void onHeartbeat(const Message& m) { self().onDefault(m); }
  void onHello(const Message& m) { self().onDefault(m); }
  void onWelcome(const Message& m) { self().onDefault(m); }
  void onBye(const Message& m) { self().onDefault(m); }
  void onJob(const Message& m) { self().onDefault(m); }
  void onResult(const Message& m) { self().onDefault(m); }
  void onBatchJob(const Message& m) { self().onDefault(m); }
  void onBatchResult(const Message& m) { self().onDefault(m); }

private:
  Base& self() { return *static_cast<Base*>(this); }

  Socket& socket_;
};

} // namespace stateline

}// namespace comms
