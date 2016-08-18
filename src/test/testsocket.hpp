#pragma once

#include "comms/socket.hpp"

namespace stateline { namespace test {

class TestSocket : public comms::Socket
{
public:
  TestSocket(zmq::context_t& ctx, zmq::socket_type type, const std::string& name)
    : Socket{ctx, type, name}
  {
  }

  comms::Message recv()
  {
    // Ignore heartbeats
    for (;;)
    {
      const auto msg = Socket::recv();
      if (msg.subject != comms::HEARTBEAT)
        return msg;
    }
  }
};

} }
