//!
//! Contains the functions to send data across ZMQ.
//!
//! \file comms/transport.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!
#include <iomanip>
#include <sstream>
#include <random>

#include <glog/logging.h>

#include "comms/transport.hpp"
#include "comms/serial.hpp"


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
      VLOG(2) << "Message Part Received: " << result;
      return result;
    }

    bool sendString(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());

      VLOG(2) << "Message Part Sending: " << string;
      return socket.send(message);
    }

    bool sendStringPart(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());

      return socket.send(message, ZMQ_SNDMORE);
    }

    Message receive(zmq::socket_t& socket)
    {
      VLOG(1) << "Receiving Message...";
      std::vector<std::string> address;
      std::string frame = receiveString(socket);
      // Do we have an address?
      if (frame.compare("") == 0)
      {
        VLOG(1) << "Message contains no address";
      }
      else
      {
        VLOG(1) << "Message has address:";
        while (frame.compare("") != 0)
        {
          address.push_back(frame);
          frame = receiveString(socket);
        }
      }
      // address is a stack, so reverse it to get the right way around
      std::reverse(address.begin(), address.end());

      // We've just read the delimiter, so now get subject
      VLOG(1) << "Reading subject:";
      auto subjectString = receiveString(socket);
      VLOG(1) << "--as string: " << subjectString << " of length " << subjectString.size();
      for (auto c : subjectString)
      {
        VLOG(1) << "----char " << c << "uint: " << uint(c);
      }
      // VLOG(1) << "--as uint: " << uint(&subjectString.c_str()[0]);
      const char* chars = subjectString.c_str();
      //the underlying representation is (explicitly) an int so fairly safe
      Subject subject = (Subject)detail::unserialise<std::uint32_t>(subjectString);
      VLOG(1) << "--as Subject: " << subject;
      VLOG(1) << "Reading data:";
      std::vector<std::string> data;
      while (true)
      {
        int isMore = 0;
        size_t moreSize = sizeof(isMore);
        socket.getsockopt(ZMQ_RCVMORE, &isMore, &moreSize);
        if (!isMore)
          break;
        data.push_back(receiveString(socket));
      }
      return Message(address, subject, data);
    }

    void send(zmq::socket_t& socket, const Message& message)
    {
      // Remember we're using the vector as a stack, so iterate through the
      // address in reverse.
      for (auto it = message.address.rbegin(); it != message.address.rend(); ++it)
      {
        sendStringPart(socket, *it);
      }

      // Send delimiter
      sendStringPart(socket, "");

      // Send subject, then data if there is any
      auto subjectString = detail::serialise<std::uint32_t>(message.subject);
      uint dataSize = message.data.size();
      if (dataSize > 0)
      {
        // The subject
        sendStringPart(socket, subjectString);

        // The data -- multipart
        for (auto it = message.data.begin(); it != std::prev(message.data.end()); ++it)
        {
          sendStringPart(socket, *it);
        }

        // final or only part
        sendString(socket, message.data.back());
      }
      else
      {
        // The subject
        sendString(socket, subjectString);
      }
    }

    void send(zmq::socket_t& socket, Message&& message)
    {
      // Remember we're using the vector as a stack, so iterate through the
      // address in reverse.
      for (auto it = message.address.rbegin(); it != message.address.rend(); ++it)
      {
        sendStringPart(socket, std::move(*it));
      }

      // Send delimiter
      sendStringPart(socket, "");

      // Send subject, then data if there is any
      auto subjectString = detail::serialise<std::uint32_t>(message.subject);
      uint dataSize = message.data.size();
      if (dataSize > 0)
      {
        // The subject
        sendStringPart(socket, std::move(subjectString));

        // The data -- multipart
        for (auto it = message.data.begin(); it != std::prev(message.data.end()); ++it)
        {
          sendStringPart(socket, std::move(*it));
        }

        // final or only part
        sendString(socket, std::move(message.data.back()));
      }
      else
      {
        // The subject
        sendString(socket, std::move(subjectString));
      }
    }

    std::string randomSocketID()
    {
      // Inspired by zhelpers.hpp
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, 0x10000);
      std::stringstream ss;
      ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << dis(gen) << "-" << std::setw(4) << std::setfill('0')
          << dis(gen);
      return ss.str();
    }

    void setSocketID(const std::string& id, zmq::socket_t & socket)
    {
      socket.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
    }

  } // namespace comms
} // namespace stateline
