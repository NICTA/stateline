#pragma once

#include <functional>
#include <zmq.hpp>

#include "comms/messages.hpp"

namespace stateline
{
  namespace comms
  {

    // Wraps a zeromq socket with some extra goodies
    class Socket
    {
      public:
        Socket(zmq::context_t& context, int type, const std::string& name);
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        void connect(const std::string& address);
        void bind(const std::string& address);
        void send(const Message& m);
        Message receive();
        void setFallback(const std::function<void(const Message& m)>& sendCallback);
        void setLinger(int l);
        void setIdentifier();
        void setIdentifier(const std::string& id);
        std::string name() const;

      private:
        zmq::socket_t socket_;
        std::string name_;
        std::function<void(const Message& m)> onFailedSend_;

        friend class SocketRouter;
    };
  }
}
