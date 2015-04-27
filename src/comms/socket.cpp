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

#include <sstream>
#include <iomanip>
#include <glog/logging.h>

namespace stateline
{
  namespace comms
  {
    std::string receiveString(zmq::socket_t & socket)
    {
      zmq::message_t message;
      std::string result = "";
      try
      {
        socket.recv(&message);
        result = std::string(static_cast<char*>(message.data()), message.size());
      }
      catch(const zmq::error_t& e) 
      {
        VLOG(1) << "ZMQ receive has thrown with type " << e.what();
        throw;
      }
      return result;
    }

    bool sendString(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());

      return socket.send(message);
    }

    bool sendStringPart(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());

      return socket.send(message, ZMQ_SNDMORE);
    }

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
      VLOG(5) << "Socket " << name_ << " sending " << m;

      try
      {
        // Remember we're using the vector as a stack, so iterate through the
        // address in reverse.
        for (auto it = m.address.rbegin(); it != m.address.rend(); ++it)
        {
          sendStringPart(socket_, *it);
        }

        // Send delimiter
        sendStringPart(socket_, "");

        // Send subject, then data if there is any
        auto subjectString = std::to_string(m.subject);
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
      catch(...)
      {
        if (onFailedSend_)
        {
          onFailedSend_(m);
        }
        else
        {
          throw;
        }
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
      //the underlying representation is (explicitly) an int so fairly safe
      Subject subject = (Subject)std::stoi(subjectString);
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

      Message message{std::move(address), subject, std::move(data)};
      VLOG(5) << "Socket " << name_ << " received " << message;
      return message;
    }

    // Options
    void Socket::setFallback(const std::function<void(const Message& m)>& sendCallback)
    {
      onFailedSend_ = sendCallback;
    }

    void Socket::setLinger(int l)
    {
      socket_.setsockopt(ZMQ_LINGER, &l, sizeof(int));
    }

    std::string Socket::name() const
    {
      return name_;
    }

    void Socket::setIdentifier()
    {
      // Inspired by zhelpers.hpp
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, 0x10000);
      std::stringstream ss;
      ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << dis(gen) << "-" << std::setw(4) << std::setfill('0')
          << dis(gen);
      setIdentifier(ss.str());
    }

    void Socket::setIdentifier(const std::string& id)
    {
      socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
    }

  } // namespace comms
} // namespace stateline
