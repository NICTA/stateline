//! Implementation of ZMQ socket wrappers.
//!
//! \file src/comms/socket.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/socket.hpp"

#include "common/logging.hpp"

#include <sstream>
#include <iomanip>
#include <random>

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
  catch(const zmq::error_t& err)
  {
    LOG(FATAL) << "Socket '" << name() << "' could not bind to " << address << " "
      << pprint("err", err.what());
  }
}

bool SocketBase::send(const std::string& address, const std::string& data)
{
  // Send the message
  try
  {
    if (!address.empty())
      socket_.send(address.c_str(), address.size(), ZMQ_SNDMORE);
    socket_.send(data.c_str(), data.size());

    hb_.updateLastSendTime(address);
    return true;
  }
  catch(const zmq::error_t& err)
  {
    LOG(ERROR) << "Socket '" << name() << "' could not send to " << address << " "
      << pprint("err", err.what());

    // TODO: disconnect heartbeat immediately
    return false;
  }
}

std::pair<std::string, std::string> SocketBase::recv()
{
  zmq::message_t msg1, msg2;
  std::string address, data;

  // Get the address and data. We expect either one frame (data only) or two frames
  // (address and data)
  socket_.recv(&msg1);
  if (msg1.more())
  {
    socket_.recv(&msg2);
    assert(!msg2.more());

    address = std::string{msg1.data<char>(), msg1.size()};
    data = std::string{msg2.data<char>(), msg2.size()};
  }
  else
  {
    data = std::string{msg1.data<char>(), msg1.size()};
  }

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
  SL_LOG(TRACE) << "Socket " << name() << " sending " << m;

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
  SL_LOG(TRACE) << "Socket " << name() << " received " << m;
  return m;
}

} }
