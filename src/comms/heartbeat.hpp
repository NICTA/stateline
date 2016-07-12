//! Interface for heartbeat monitoring.
//!
//! \file comms/heartbeat.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/message.hpp"
#include "comms/settings.hpp"

#include <chrono>
#include <unordered_map>

namespace stateline { namespace comms {

enum class DisconnectReason
{
  USER_REQUESTED, // e.g. receiving a BYE message
  TIMEOUT // heartbeat timeout
};

//! Manages the liveness of connections. This class has two roles. It keeps track
//! of heartbeats sent by connections and also triggers callbacks to send heartbeats.
//!
class Heartbeat
{
public:
  using Clock = std::chrono::high_resolution_clock;

  using HeartbeatCallback = std::function<void(const std::string&)>;
  using DisconnectCallback = std::function<void(const std::string&)>;

  //! Construct a heartbeat manager.
  //!
  Heartbeat();

  void connect(const std::string& addr, std::chrono::seconds timeout);

  void disconnect(const std::string& addr, DisconnectReason reason = DisconnectReason::USER_REQUESTED);

  std::size_t numConnections() const { return conns_.size(); }

  Clock::time_point lastSendTime(const std::string& addr) const { return conns_.at(addr).lastSendTime; }
  Clock::time_point lastRecvTime(const std::string& addr) const { return conns_.at(addr).lastRecvTime; }

  void updateLastSendTime(const std::string& addr);
  void updateLastRecvTime(const std::string& addr);

  bool hasTimeout() const { return !conns_.empty(); }
  Clock::time_point nextTimeout() const { return nextTimeout_; }

  //! Call this to handle timeouts and send heartbeats.
  //!
  void idle();

  void bindHeartbeat(HeartbeatCallback callback) { heartbeatCallback_ = callback; }

  void bindDisconnect(DisconnectCallback callback) { disconnectCallback_ = callback; }

private:
  struct Connection
  {
    std::chrono::milliseconds interval;
    Clock::time_point lastSendTime;
    Clock::time_point lastRecvTime;

    Connection(std::chrono::milliseconds interval)
      : interval{interval}
      , lastRecvTime{Clock::now()}
    {
    }
  };

  using ConnMap = std::unordered_map<std::string, Connection>;

  void sendHeartbeat(ConnMap::value_type& kv);

  ConnMap conns_;
  Clock::time_point nextTimeout_; // TODO: some sort of priority queue?
  HeartbeatCallback heartbeatCallback_;
  DisconnectCallback disconnectCallback_;
};

} }
