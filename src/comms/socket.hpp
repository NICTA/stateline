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

#include <functional>

#include <zmq.hpp>

namespace stateline
{

namespace comms
{

enum { NO_LINGER = 0 };

//! Thin wrapper around a ZMQ socket.
class SocketBase
{
public:
  SocketBase(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger);

  SocketBase(const SocketBase&) = delete;
  SocketBase& operator=(const SocketBase&) = delete;

  const std::string& name() const { return name_; }
  zmq::socket_t& zmqSocket() { return socket_; }

  void connect(const std::string& address);
  void bind(const std::string& address);

  template <class Callback>
  void setFallback(const Callback& callback) { onFailedSend_ = callback; };
  void onFailedSend(const Message& m) { onFailedSend_(m); }

private:
  zmq::socket_t socket_;
  std::string name_;
  std::function<void(const Message& m)> onFailedSend_;
};

//! High-level wrapper around a ZMQ socket. This socket is used for internal messages.
struct Socket : public SocketBase
{
  Socket(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger = NO_LINGER);

  void send(const Message& m);
  Message recv();

  void setIdentity();
  void setIdentity(const std::string& id);
};

//! High-level wrapper around a ZMQ_STREAM socket. This socket is used for external messages.
struct RawSocket : public SocketBase
{
  RawSocket(zmq::context_t& ctx, std::string name, int linger = NO_LINGER);

  void send(const Message& m);
  Message recv();
};

} // namspace comms

} // namespace stateline
