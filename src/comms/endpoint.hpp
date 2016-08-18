//! Provides a message handling layer above a socket.
//!
//! \file comms/endpoint.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "common/logging.hpp"
#include "comms/socket.hpp"

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
    socket.heartbeats().bindHeartbeat(
        [this](const std::string& addr) { onHeartbeatSend(addr); });
    socket.heartbeats().bindDisconnect(
        [this](const std::string& addr, DisconnectReason reason) { onHeartbeatDisconnect(addr, reason); });
  }

  Socket& socket() { return socket_; }

  void accept()
  {
    handle(socket_.recv());
  }

  void handle(const Message& m)
  {
    SL_LOG(DEBUG) << "Handling message " << pprint("msg", m);

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

      case BYE:
        self().onBye(m);
        break;

      case JOB:
        self().onJob(m);
        break;

      case RESULT:
        self().onResult(m);
        break;

      case BATCH_JOB:
        self().onBatchJob(m);
        break;

      case BATCH_RESULT:
        self().onBatchResult(m);
        break;

      default:
        SL_LOG(WARNING) << "Received message with unknown subject "
          << pprint("subject", m.subject);

        self().onDefault(m);
        break;
    }
  }

  void onDefault(const Message&) { }
  void onHeartbeat(const Message& m) { self().onDefault(m); }
  void onHello(const Message& m) { self().onDefault(m); }
  void onWelcome(const Message& m) { self().onDefault(m); }
  void onBye(const Message& m) { self().onDefault(m); }
  void onJob(const Message& m) { self().onDefault(m); }
  void onResult(const Message& m) { self().onDefault(m); }
  void onBatchJob(const Message& m) { self().onDefault(m); }
  void onBatchResult(const Message& m) { self().onDefault(m); }

  void onHeartbeatSend(const std::string& addr)
  {
    // Default behaviour is to send an empty heartbeat msg
    SL_LOG(TRACE) << "Sending empty heartbeat " << pprint("addr", addr);
    socket_.send({ addr, HEARTBEAT, "" });
  }

  void onHeartbeatDisconnect(const std::string&, DisconnectReason) { }

  void forwardMessage(Socket& s, const Message &m)
  {
    SL_LOG(DEBUG) << "Forwarding message to " << s.name() << " " << pprint("msg", m);
    s.send(m);
  }

  void idle()
  {
    socket_.heartbeats().idle();
  }

private:
  Base& self() { return *static_cast<Base*>(this); }

  Socket& socket_;
};

} }
