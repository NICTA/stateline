//!
//! Contains the interface for representing network messages.
//!
//! \file comms/messages.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <vector>

#include "datatypes.hpp"

namespace stateline
{
  namespace comms
  {
    //! Represents the header of a message containing addresses.
    using Address = std::vector<std::string>;

    //! Defines the bytes actually sent in the subject frame messages.
    //! Allows us to use a switch statement on the subject.
    //!
    // TODO: make into a enum class
    enum Subject
    {
      HELLO = 0,
      HEARTBEAT = 1,
      WORK = 2,
      GOODBYE = 3,
      Size
    };

    //! Define valid messages to send between delegators and workers.
    //!
    struct Message
    {
      //! Constructor to build a message.
      //!
      //! \param address The address to send the message to.
      //! \param subject The subject of the message (eg. HELLO, JOB etc).
      //! \param data A vector of data to send in the message. Each element of
      //!          the vector is sent as a separate frame.
      //!
      Message(Address address, Subject subject, std::vector<std::string> data = {});

      //! Create a new message with no address.
      //!
      //! \param subject The subject of the message (eg. HELLO, JOB etc).
      //! \param data A vector of data to send in the message. Each element of
      //!          the vector is sent as a separate frame.
      //!
      Message(Subject subject, std::vector<std::string> data = {});

      //! Equality comparator for testing purposes.
      //!
      //! \param m The message to compare with.
      //! \return True if the message is equal to this message.
      //!
      bool operator==(const Message& m) const;

      //! Allows a message to be printed with std::cout.
      //!
      //! \param os the output stream.
      //! \param m the message object.
      //!
      friend std::ostream& operator<<(std::ostream& os, const Message& m);

      //! The destination address of this message.
      Address address;

      //! The subject of the message (e.g. HELLO).
      Subject subject;

      //! The data that this message contains.
      std::vector<std::string> data;
    };

    //! Convert an address to a string.
    //!
    //! \param addr The address to convert.
    //! \return The address as a single string.
    //!
    std::string addressAsString(const Address& address);

    //! Print a message for logging and debugging purposes.
    //!
    //! \param os The output stream.
    //! \param m The message to print.
    //!
    std::ostream& operator<<(std::ostream& os, const Message& m);

  } // namespace comms
} // namespace stateline

