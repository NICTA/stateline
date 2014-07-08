//!
//! \file comms/messages.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/messages.hpp"

#include <string>
#include <vector>
#include <map>
#include <glog/logging.h>
#include <zmq.hpp>

namespace stateline
{
  namespace comms
  {
    //! Convert a subject into a string.
    //!
    //! \param s The subject to convert.
    //! \return The string representation of the subject.
    //!
    std::string subjectString(Subject s)
    {
      static std::map<Subject, std::string> names
      {
        { HELLO, "HELLO" },
        { HEARTBEAT, "HEARTBEAT" },
        { PROBLEMSPEC, "PROBLEMSPEC" },
        { JOBREQUEST, "JOBREQUEST" },
        { JOB, "JOB" },
        { JOBSWAP, "JOBSWAP" },
        { ALLDONE, "ALLDONE" },
        { GOODBYE, "GOODBYE" }
      };
      return names[s];
    }

    Message::Message(const std::vector<std::string>& addr, const Subject& subj, const std::vector<std::string>& d)
        : address(addr), subject(subj), data(d)
    {
    }

    Message::Message(const Subject& subj, const std::vector<std::string>& d)
        : subject(subj), data(d)
    {
    }

    Message::Message(const std::vector<std::string>& addr, const Subject& subj)
        : address(addr), subject(subj)
    {
    }

    Message::Message(const Subject& subj)
        : subject(subj)
    {
    }

    bool Message::operator ==(const Message& m) const
    {
      return (address == m.address) && (subject == m.subject) && (data == m.data);
    }

    std::string addressAsString(const std::vector<std::string>& addr)
    {
      // Concatenate the vector of addresses together with ':' as a delimiter
      std::string buffer;
      uint i = addr.size();
      while (i--)
      {
        buffer.append(addr[i]);
        if (i > 0) buffer.append(":");
      }
      return buffer;
    }

    std::ostream& operator<<(std::ostream& os, const Message& m)
    {
      os << "|" << addressAsString(m.address) << "|" << subjectString(m.subject) << "|<" << m.data.size() << " data frames>|";
      return os;
    }

    void print(const Message& m)
    {
      VLOG(3) << "Received:" << m;
    }

  } // namespace comms
} // namespace obsidian

