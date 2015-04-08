//!
//! Heartbeat routers that deal with client heartbeating
//! sending and receiving.
//!
//! \file comms/clientheartbeat.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/settings.hpp"
#include "comms/socket.hpp"

namespace stateline
{
  namespace comms
  {
    //! Heartbeat sockets talk on these addresses.
    const std::string CLIENT_HB_SOCKET_ADDR = "inproc://clienthb";

    //! Handles heartbeating on the client side. Client heartbeat assumes only
    //! one connection (to the server), so sends and receives to one source only.
    //!
    class ClientHeartbeat
    {
    public:
      //! Create a new client heartbeat thread.
      //!
      //! \param context The ZMQ context.
      //! \param settings Heartbeat settings.
      //!
      ClientHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings, bool& running);

      void start();

      //! Cleanup resources used by the heartbeat thread.
      //!
      ~ClientHeartbeat();

    private:
      Socket socket_;
      SocketRouter router_;

      uint msPollRate_;

      hrc::time_point lastSendTime_;
      hrc::time_point lastReceivedTime_;

      bool running_;
    };
  } // namespace comms
} // namespace stateline

