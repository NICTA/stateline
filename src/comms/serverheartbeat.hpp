//!
//! Heartbeat routers that deal with server heartbeating sending and receiving.
//!
//! \file comms/serverheartbeat.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <set>
#include <map>

#include "messages.hpp"
#include "router.hpp"
#include "settings.hpp"
#include "socket.hpp"

namespace stateline
{
  namespace comms
  {
    //! Heartbeat sockets talk on these addresses.
    const std::string SERVER_HB_SOCKET_ADDR = "inproc://serverhb";

    //! A set of clients being monitored for heartbeating.
    typedef std::set<std::string> HBClients;

    //! A map of clients to when the last heartbeat arrived from that client.
    typedef std::map<std::string, hrc::time_point> HBMap;

    //! Handles heartbeating on the server side. Server heartbeat assumes that
    //! it has multiple connections from different clients, so sends to all
    //! of them, and keeps track of who is connected.
    //!
    class ServerHeartbeat
    {
    public:
      //! Create a new server heartbeat thread.
      //!
      //! \param context The ZMQ context.
      //! \param settings Heartbeat settings.
      //!
      ServerHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings, bool& running);

      void start();

      //! Cleanup resources used by the heartbeat thread.
      //!
      ~ServerHeartbeat();

    private:
      Socket socket_;
      SocketRouter router_;

      uint msPollRate_;

      HBClients clients_;
      HBMap lastHeartbeats_;

      hrc::time_point lastSendTime_;

      bool& running_;
    };

  } // namespace comms
} // namespace stateline
