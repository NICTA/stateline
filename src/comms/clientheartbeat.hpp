//!
//! Heartbeat routers that deal with client heartbeating
//! sending and receiving.
//!
//! \file comms/clientheartbeat.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Project
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/settings.hpp"
// Standard Library
#include <chrono>
//Prerequisites

//! Heartbeat sockets talk on these addresses.
const std::string CLIENT_HB_SOCKET_ADDR = "inproc://clienthb";

typedef std::chrono::high_resolution_clock hrc;

namespace stateline
{
  namespace comms
  {
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
      ClientHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings);

      //! Cleanup resources used by the heartbeat thread.
      //!
      ~ClientHeartbeat();

      //! Start the heartbeat thread.
      //!
      void start();

      //! Stop the heartbeat thread.
      //!
      void stop();

    private:
      const uint msFrequency_;
      const int msPollingFrequency_;
      const uint msTimeout_;
      SocketRouter router_;
      hrc::time_point lastSendTime_;
      hrc::time_point lastReceivedTime_;
    };
  } // namespace comms
} // namespace obsidian

