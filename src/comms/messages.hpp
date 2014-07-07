//!
//! Contains the functions to seralise and unserialise GDF datatypes.
//!
//! \file comms/messages.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <string>
#include <vector>
//Prerequisites
#include <boost/range/adaptor/reversed.hpp>
// Project
#include "datatypes.hpp"

namespace stateline
{
  namespace comms
  {
    //! Defines the bytes actually sent in the subject frame of GDF messages.
    //! Allows us to use a switch statement on the subject.
    //!
    enum Subject : uint
    {
      HELLO = 0,
      HEARTBEAT = 1,
      PROBLEMSPEC = 2,
      JOBREQUEST = 3,
      JOB = 4,
      JOBSWAP = 5,
      ALLDONE = 6,
      GOODBYE = 7
    };

    //! Identifies a particular message code, defined as a Subject enum.
    //! 
    //! \param s The subject enum being queried (a uint really).
    //! \return The name of the corresponding message.
    //!
    std::string subjectString(Subject s);

    //! Define valid messages to send on the GDFP-SW (GDF Server-Worker Protocol)
    //!
    class Message
    {
    public:

      //! Constructor to build a message.
      //! 
      //! \param addr The address to send the message to.
      //! \param subj The subject of the message (eg. HELLO, JOB etc).
      //! \param d A vector of data to send in the message. Each element of
      //!          the vector is sent as a separate frame.
      //!
      Message(const std::vector<std::string>& addr, const Subject& subj,
          const std::vector<std::string>& d);

      //! Create a new message with no address.
      //!
      //! \param subj The subject of the message (eg. HELLO, JOB etc).
      //! \param d A vector of data to send in the message. Each element of
      //!          the vector is sent as a separate frame.
      //!
      //!
      Message(const Subject& subj, const std::vector<std::string>& d);

      //! Create a new message with no data.
      //! 
      //! \param addr The address to send the message to.
      //! \param subj The subject of the message (eg. HELLO, JOB etc).
      //!
      Message(const std::vector<std::string>& addr, const Subject& subj);

      //! Constructor to build a message with no data or address.
      //! 
      //! \param subj The subject of the message (eg. HELLO, JOB etc).
      //!
      Message(const Subject& subj);

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
      std::vector<std::string> address;

      //! The subject of the message (e.g. HELLO).
      Subject subject;

      //! The data that this message contains.
      std::vector<std::string> data;
    };

    //! Print a message for logging and debugging purposes.
    //!
    //! \param os The output stream.
    //! \param m The message to print.
    //!
    std::ostream& operator<<(std::ostream& os, const Message& m);

    //! Convert an address to a string.
    //!
    //! \param addr The address to convert.
    //! \return The address as a single string.
    //!
    std::string addressAsString(const std::vector<std::string>& addr);

    //! Print a message for logging and debugging purposes.
    //!
    //! \param m The message to print.
    //!
    void print(const Message& m);

  } // namespace comms
} // namespace obsidian

