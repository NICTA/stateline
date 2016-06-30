//! Contains a generic message structure.
//!
//! \file comms/socket.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <cstdint>
#include <ostream>
#include <string>

namespace stateline
{

namespace comms
{

//! Represents all the message types used in Stateline.
enum Subject : std::uint8_t // TODO: make this an enum class
{
  HEARTBEAT = 0,
  HELLO = 1,
  BYE = 2,
  JOB = 3,
  RESULT = 4,
  BATCH_JOB = 5,
  BATCH_RESULT = 6,
  SIZE
};

//! Represents a network message.
struct Message
{
  // Put fields in this order for optimal struct packing.
  std::string address; //! Identity of the source/destination socket.
  std::string data; //! Payload data.
  Subject subject; //! The type of message this is.

  //! Construct a new message.
  //! \param address The address of the sender or receiver.
  //! \param subject The type of message.
  //! \param data The payload data.
  Message(std::string address, Subject subject, std::string data)
    : address{std::move(address)}
    , data{std::move(data)}
    , subject{subject}
  {
  }
};

std::ostream& operator<<(std::ostream& os, const Message& m);

}

}
