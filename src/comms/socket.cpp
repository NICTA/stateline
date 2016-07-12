//! Implementation of ZMQ socket wrappers.
//!
//! \file comms/socket.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/socket.hpp"

#include <sstream>
#include <iomanip>
#include <easylogging/easylogging++.h>
#include <random>

#include <zmq_addon.hpp>

namespace stateline { namespace comms {

SocketBase::SocketBase(zmq::context_t& context, zmq::socket_type type, std::string name, int linger)
  : socket_{context, type}
  , name_{std::move(name)}
{
  socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
}

void SocketBase::connect(const std::string& address)
{
  socket_.connect(address.c_str());
}

void SocketBase::bind(const std::string& address)
{
  try
  {
    socket_.bind(address.c_str());
  }
  catch(...)
  {
    LOG(FATAL) << "Could not bind to " << address << ". Address already in use";
  }
}

bool SocketBase::send(const std::string& address, const std::string& data)
{
  zmq::multipart_t msg;
  if (!address.empty())
    msg.addstr(address);
  msg.addstr(data);

  // Send the message
  if (msg.send(socket_))
  {
    hb_.updateLastSendTime(address);
    return true;
  }

  // TODO: disconnect heartbeat immediately
  return false;
}

std::pair<std::string, std::string> SocketBase::recv()
{
  // Receive a multipart message
  zmq::multipart_t msg{socket_};
  assert(msg.size() == 1 || msg.size() == 2);

  // Get the address and data
  auto address = msg.size() == 1 ? "" : msg.popstr();
  auto data = msg.popstr();

  hb_.updateLastRecvTime(address);

  return {std::move(address), std::move(data)};
}


void SocketBase::setIdentity()
{
  // Inspired by zhelpers.hpp
  std::random_device rd;
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<> dis{0, 0x10000};
  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << dis(gen) << "-" << std::setw(4) << std::setfill('0')
      << dis(gen);

  setIdentity(ss.str());
}

void SocketBase::setIdentity(const std::string& id)
{
  socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
}

void SocketBase::startHeartbeats(const std::string& address, std::chrono::seconds timeout)
{
  hb_.connect(address, timeout);
}

Socket::Socket(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger)
  : SocketBase(ctx, type, std::move(name), linger)
{
}

bool Socket::send(const Message& m)
{
  VLOG(5) << "Socket " << name() << " sending " << m;

  // Pack the subject and the data together
  std::string buffer(sizeof(Subject) + m.data.size(), ' ');
  memcpy(&buffer[0], reinterpret_cast<const char*>(&m.subject), sizeof(Subject));
  memcpy(&buffer[0] + sizeof(Subject), m.data.data(), m.data.size());

  return SocketBase::send(m.address, buffer);
}

Message Socket::recv()
{
  auto msg = SocketBase::recv();
  assert(msg.second.size() >= sizeof(Subject));

  // Unpack the subject and data
  const auto subject = *reinterpret_cast<Subject*>(&msg.second[0]);
  std::string data{msg.second.data() + sizeof(Subject), msg.second.data() + msg.second.size()};

  Message m{msg.first, subject, std::move(data)};
  VLOG(5) << "Socket " << name() << " received " << m;
  return m;
}

} }
