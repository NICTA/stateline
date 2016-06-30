#include "comms/message.hpp"

namespace stateline
{

namespace comms
{

std::ostream& operator<<(std::ostream& os, const Message& m)
{
  os << "|" << m.address << "|<" << m.data.size() << " data frames>|";
  return os;
}

} // namespace comms

} // namespace stateline
