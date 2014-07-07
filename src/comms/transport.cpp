//!
//! Contains the functions to communicate GDF datatypes across zmq in accordance
//! with the GDFP-SW (GDF Server-Worker protocol)
//!
//! \file comms/transport.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/transport.hpp"
#include <iomanip>
#include <boost/lexical_cast.hpp>

#include <random>

namespace stateline
{
  namespace comms
  {
    std::string receiveString(zmq::socket_t & socket)
    {
      //shamelessly taken from zmq guide examples, zhelpers.hpp
      zmq::message_t message;
      socket.recv(&message);
      return std::string(static_cast<char*>(message.data()), message.size());
    }

    bool sendString(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());
      bool rc = socket.send(message);
      return (rc);
    }

    bool sendStringPart(zmq::socket_t & socket, const std::string & string)
    {
      // Taken from zhelpers.hpp
      zmq::message_t message(string.size());
      memcpy(message.data(), string.data(), string.size());
      bool rc = socket.send(message, ZMQ_SNDMORE);
      return (rc);
    }

    Message receive(zmq::socket_t& socket)
    {
      std::vector<std::string> address;
      std::string frame = stateline::comms::receiveString(socket);
      // Do we have an address?
      while (frame.compare("") != 0)
      {
        address.push_back(frame);
        frame = stateline::comms::receiveString(socket);
      }
      // address is a stack, so reverse it to get the right way around
      std::reverse(address.begin(), address.end());

      // We've just read the delimiter, so now get subject
      auto subjectString = stateline::comms::receiveString(socket);
      //the underlying representation is (explicitly) an int so fairly safe
      Subject subject = (Subject) boost::lexical_cast<uint>(subjectString);
      std::vector<std::string> data;
      while (true)
      {
        int isMore = 0;
        size_t moreSize = sizeof(isMore);
        socket.getsockopt(ZMQ_RCVMORE, &isMore, &moreSize);
        if (!isMore)
          break;
        data.push_back(stateline::comms::receiveString(socket));
      }
      return Message(address, subject, data);
    }

    void send(zmq::socket_t& socket, const Message& message)
    {
      // remember we're using the vector as a stack
      for (auto a : boost::adaptors::reverse(message.address))
      {
        stateline::comms::sendStringPart(socket, a);
      }
      // Send delimiter
      stateline::comms::sendStringPart(socket, "");

      // Send subject, then data if there is any
      auto subjectString = boost::lexical_cast<std::string>(message.subject);
      uint dataSize = message.data.size();
      if (dataSize > 0)
      {
        // The subject
        stateline::comms::sendStringPart(socket, subjectString);
        // The data -- multipart
        for (auto it = message.data.begin(); it != std::prev(message.data.end()); it++)
        {
          stateline::comms::sendStringPart(socket, *it);
        }
        // final or only part
        stateline::comms::sendString(socket, message.data[dataSize - 1]);
      } else
      {
        // The subject
        stateline::comms::sendString(socket, subjectString);
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
} // namespace obsidian
