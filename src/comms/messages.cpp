//!
//! \file comms/messages.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/messages.hpp"

#include <algorithm>
#include <ostream>

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
      switch (s)
      {
        case HELLO: return "HELLO";
        case HEARTBEAT: return "HEARTBEAT";
        case WORK: return "WORK";
        case GOODBYE: return "GOODBYE";
        default: return "UNKNOWN";
      }
    }

    Message::Message(Address address, Subject subject, std::vector<std::string> data)
      : address(std::move(address)), subject(std::move(subject)), data(std::move(data))
    {
    }

    Message::Message(Subject subject, std::vector<std::string> data)
        : subject(std::move(subject)), data(std::move(data))
    {
    }

    bool Message::operator==(const Message& m) const
    {
      return address == m.address && subject == m.subject && data == m.data;
    }

    std::string addressAsString(const Address& address)
    {
      // Concatenate the vector of addresses together with ':' as a delimiter
      std::string result;
      if (!address.empty()) {
        result += address.back();
        std::for_each(address.rbegin() + 1, address.rend(),
            [&](const std::string& addr) { result += ":" + addr; });
      }
      return result;
    }

    std::ostream& operator<<(std::ostream& os, const Message& m)
    {
      os << "|" << addressAsString(m.address) << "|" << subjectString(m.subject) << "|<" << m.data.size() << " data frames>|";
      return os;
    }

  } // namespace comms
} // namespace stateline

