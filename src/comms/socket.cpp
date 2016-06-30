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

namespace stateline
{

namespace comms
{

SocketBase::SocketBase(zmq::context_t& context, zmq::socket_type type, std::string name, int linger)
  : socket_{context, type}
  , name_{std::move(name)}
  , onFailedSend_{[](const Message&) { throw std::runtime_error{"Failed to send message"}; }}
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

Socket::Socket(zmq::context_t& ctx, zmq::socket_type type, std::string name, int linger)
  : SocketBase(ctx, type, std::move(name), linger)
{
}

void Socket::send(const Message& m)
{
  VLOG(5) << "Socket " << name() << " sending " << m;

  zmq::multipart_t msg;
  if (!m.address.empty())
    msg.addstr(m.address);
  msg.addtyp(m.subject);
  msg.addstr(m.data);

  // Send the message
  if (!msg.send(zmqSocket()))
    onFailedSend(m);
}

Message Socket::recv()
{
  // Receive a multipart message
  zmq::multipart_t msg{zmqSocket()};
  assert(msg.size() == 2 || msg.size() == 3);

  // Get the address, subject and data in order
  auto address = msg.size() == 2 ? "" : msg.popstr();
  auto subject = msg.poptyp<Subject>();
  auto data = msg.popstr();

  Message m{std::move(address), subject, std::move(data)};
  VLOG(5) << "Socket " << name() << " received " << m;
  return m;
}

void Socket::setIdentity()
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

void Socket::setIdentity(const std::string& id)
{
  zmqSocket().setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
}

} // namespace comms

} // namespace stateline
