#pragma once

#include <proto/messages.pb.h>

namespace stateline { namespace comms {

template <class Protobuf>
std::string protobufToString(const Protobuf& protobuf)
{
  std::string buf;
  protobuf.SerializeToString(&buf);
  return buf;
}

template <class Protobuf>
Protobuf stringToProtobuf(const std::string& str)
{
  Protobuf protobuf;
  protobuf.ParseFromString(str);
  return protobuf;
}

} }
