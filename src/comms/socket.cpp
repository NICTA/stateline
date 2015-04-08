//!
//! Contains the implementation of the ZMQ socket wrapper.
//!
//! \file comms/socket.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/socket.hpp"

#include <glog/logging.h>

#include "comms/transport.hpp"
#include "comms/serial.hpp"

namespace stateline
{
  namespace comms
  {
    Socket::Socket(zmq::context_t& context, int type, const std::string& name)
      : socket_(context, type),
        name_(name),
        onFailedSend_(nullptr) // TODO: default on failed send
    {
    }

    void Socket::connect(const std::string& address)
    {
      socket_.connect(address.c_str());
    }

    void Socket::bind(const std::string& address)
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

    void Socket::send(const Message& m)
    {
      VLOG(4) << "Socket " << name_ << " sending " << m;

      // Remember we're using the vector as a stack, so iterate through the
      // address in reverse.
      for (auto it = m.address.rbegin(); it != m.address.rend(); ++it)
      {
        sendStringPart(socket_, *it);
      }

      // Send delimiter
      sendStringPart(socket_, "");

      // Send subject, then data if there is any
      auto subjectString = detail::serialise<std::uint32_t>(m.subject);
      uint dataSize = m.data.size();
      if (dataSize > 0)
      {
        // The subject
        sendStringPart(socket_, subjectString);

        // The data -- multipart
        for (auto it = m.data.begin(); it != std::prev(m.data.end()); ++it)
        {
          sendStringPart(socket_, *it);
        }

        // final or only part
        sendString(socket_, m.data.back());
      }
      else
      {
        // The subject
        sendString(socket_, subjectString);
      }
    }

    Message Socket::receive()
    {
      std::vector<std::string> address;
      std::string frame = receiveString(socket_);
      // Do we have an address?
      while (frame.compare("") != 0)
      {
        address.push_back(frame);
        frame = receiveString(socket_);
      }
      // address is a stack, so reverse it to get the right way around
      std::reverse(address.begin(), address.end());

      // We've just read the delimiter, so now get subject
      auto subjectString = receiveString(socket_);
      const char* chars = subjectString.c_str();
      //the underlying representation is (explicitly) an int so fairly safe
      Subject subject = (Subject)detail::unserialise<std::uint32_t>(subjectString);
      std::vector<std::string> data;
      while (true)
      {
        int isMore = 0;
        size_t moreSize = sizeof(isMore);
        socket_.getsockopt(ZMQ_RCVMORE, &isMore, &moreSize);
        if (!isMore)
          break;
        data.push_back(receiveString(socket_));
      }

      Message message{address, subject, data};
      VLOG(4) << "Socket " << name_ << " received " << message;
      return message;
    }

    // Options
    void Socket::setFallback(const std::function<void(const Message& m)>& sendCallback)
    {
      onFailedSend_ = sendCallback;
    }

    void Socket::setLinger(int l){
      socket_.setsockopt(ZMQ_LINGER, &l, sizeof(int));
    }

    void Socket::setIdentifier(const std::string& id)
    {
      socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
    }

  } // namespace comms
} // namespace stateline
