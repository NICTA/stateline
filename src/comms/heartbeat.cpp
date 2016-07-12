//! Implementation of heartbeat monitoring.
//!
//! \file comms/heartbeat.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/heartbeat.hpp"

#include <easylogging/easylogging++.h>

namespace stateline { namespace comms {

Heartbeat::Heartbeat()
  : heartbeatCallback_{[](const auto&) { }}
  , disconnectCallback_{[](const auto&) { }}
{
}

void Heartbeat::connect(const std::string& addr, std::chrono::seconds timeout)
{
  // We send 2 heartbeats per timeout
  const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(timeout) / 2;

  conns_.emplace(std::piecewise_construct,
      std::forward_as_tuple(addr),
      std::forward_as_tuple(interval));
}

void Heartbeat::disconnect(const std::string& addr, DisconnectReason reason)
{
  switch (reason)
  {
    case DisconnectReason::USER_REQUESTED:
      VLOG(1) << addr << " disconnected by request";
      break;

    case DisconnectReason::TIMEOUT:
      VLOG(1) << addr << " disconnected by time out";
      break;
  }

  disconnectCallback_(addr);
  conns_.erase(addr);
}

void Heartbeat::updateLastSendTime(const std::string& addr)
{
  auto it = conns_.find(addr);
  if (it == conns_.end())
    return;

  it->second.lastSendTime = Clock::now();
}

void Heartbeat::updateLastRecvTime(const std::string& addr)
{
  auto it = conns_.find(addr);
  if (it == conns_.end())
    return;

  it->second.lastRecvTime = Clock::now();
}

void Heartbeat::idle()
{
  const auto now = Clock::now();

  // Send any outstanding heartbeats.
  nextTimeout_ = Clock::time_point::max();
  for (auto& kv : conns_)
  {
    if (kv.second.lastSendTime + kv.second.interval <= now)
      sendHeartbeat(kv);

    nextTimeout_ = std::min(nextTimeout_,
        kv.second.lastSendTime + kv.second.interval);
  }

  // Call the disconnect callback for any timeouts and remove them from our records.
  for (auto it = conns_.cbegin(); it != conns_.cend(); )
  {
    // Disconnect if they missed 2 heartbeat intervals.
    if (it->second.lastRecvTime + it->second.interval * 2 < now)
    {
      VLOG(1) << it->first << " timed out";
      disconnectCallback_(it->first);
      it = conns_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void Heartbeat::sendHeartbeat(ConnMap::value_type& kv)
{
  VLOG(4) << "Sending HEARTBEAT to " << kv.first;

  heartbeatCallback_(kv.first);
  kv.second.lastSendTime = Clock::now();
}

} }
