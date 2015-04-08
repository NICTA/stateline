#pragma once

#include "comms/messages.hpp"
#include "comms/transport.hpp"

namespace stateline
{
  namespace comms
  {

    // Wraps a zeromq socket with some extra goodies
    class Socket
    {
      public:
        Socket(zmq::context_t& c, int type, const std::string& name_); 
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        void connect(const std::string& address);
        void bind(const std::string& address);
        void send(const Message& m);
        Message receive();

        // Options
        void setFallback(const std::function<void(const Message& m)>& sendCallback); 
        void setLinger(int l){
          socket_.setsockopt(ZMQ_LINGER, &l, sizeof(int));
          }
        void setIdentity(const std::string& id);

      private:

        zmq::socket_t socket_;
        std::string name_;
        std::function<void(const Message& m)> onFailedSend_;

    };
  }
}
