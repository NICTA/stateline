//! Wrapper around a ZMQ socket.
//!
//! \file comms/socket.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "message.hpp"
#include "heartbeat.hpp"

#include <functional>

#include <zmq.hpp>

namespace stateline { namespace comms {

enum { NO_LINGER = 0 };

//! Wrapper around a ZMQ socket.
class SocketBase
{
public:
  SocketBase(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger = NO_LINGER);

  SocketBase(const SocketBase&) = delete;
  SocketBase& operator=(const SocketBase&) = delete;

  const std::string& name() const { return name_; }
  zmq::socket_t& zmqSocket() { return socket_; }

  bool send(const std::string& address, const std::string& data);
  std::pair<std::string, std::string> recv();

  void connect(const std::string& address);
  void bind(const std::string& address);

  void setIdentity();
  void setIdentity(const std::string& id);

  Heartbeat& heartbeats() { return hb_; }
  const Heartbeat& heartbeats() const { return hb_; }
  void startHeartbeats(const std::string& address, std::chrono::seconds timeout);

private:
  zmq::socket_t socket_;
  std::string name_;
  Heartbeat hb_;
};

//! High-level wrapper around a ZMQ socket. This socket is used for internal messages.
struct Socket : public SocketBase
{
  Socket(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger = NO_LINGER);

  bool send(const Message& m);
  Message recv();
};

} } // namespace stateline
