//! Interface for heartbeat monitoring.
//! \file comms/heartbeat.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/messages.hpp"
#include "comms/settings.hpp"

#include <chrono>
#include <unordered_map>

namespace stateline { namespace comms {

//! Manages the liveness of connections. This class has two roles. It keeps track
//! of heartbeats sent by connections and also triggers callbacks to send heartbeats.
//!
class Heartbeat
{
public:
  using HeartbeatCallback = std::function<void(const std::string&)>;
  using DisconnectCallback = std::function<void(const std::string&)>;

  //! Construct a heartbeat manager.
  //!
  Heartbeat();

  void connect(const std::string& addr, std::chrono::seconds interval);

  void update(const std::string& addr);

  void disconnect(const std::string& addr);

  //! Call this to handle timeouts and send heartbeats.
  //!
  std::chrono::milliseconds idle();

  void bindHeartbeat(HeartbeatCallback callback) { heartbeatCallback_ = callback; }

  void bindDisconnect(DisconnectCallback callback) { disconnectCallback_ = callback; }

private:
  using hrc = std::chrono::high_resolution_clock;

  struct Connection
  {
    std::chrono::seconds interval;
    hrc::time_point lastSendTime;
    hrc::time_point lastRecvTime;

    Connection(std::chrono::seconds interval)
      : interval{interval}, lastSendTime{hrc::now()}
    {
    }
  };

  using ConnMap = std::unordered_map<std::string, Connection>;

  void sendHeartbeat(ConnMap::value_type& kv);

  ConnMap conns_;
  HeartbeatCallback heartbeatCallback_;
  DisconnectCallback disconnectCallback_;
};

} }
