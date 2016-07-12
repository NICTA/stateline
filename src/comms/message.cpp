#include "comms/message.hpp"

namespace stateline { namespace comms {

std::string subjectToString(Subject s)
{
  switch (s)
  {
    case HELLO:
      return "HELLO";
  }

  return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, const Message& m)
{
  os << "|" << m.address << "|" << subjectToString(m.subject) <<
    "|<" << m.data.size() << " bytes>|";
  return os;
}

} // namespace comms

} // namespace stateline
