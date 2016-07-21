#include "comms/message.hpp"

namespace stateline { namespace comms {

std::string subjectToString(Subject s)
{
  switch (s)
  {
    case HEARTBEAT:
      return "HEARTBEAT";
    case HELLO:
      return "HELLO";
    case WELCOME:
      return "WELCOME";
    case BYE:
      return "BYE";
    case JOB:
      return "JOB";
    case RESULT:
      return "RESULT";
    case BATCH_JOB:
        return "BATCH_JOB";
    case BATCH_RESULT:
        return "BATCH_RESULT";
    default:
        return "UNKNOWN";
  }

  return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, const Message& m)
{
  os << "|" << m.address << "|" << subjectToString(m.subject) <<
    "|<" << m.data.size() << " bytes>|";
  return os;
}

} }
