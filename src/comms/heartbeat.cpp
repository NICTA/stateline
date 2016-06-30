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

void Heartbeat::connect(const std::string& addr, std::chrono::seconds interval)
{
  auto ret = conns_.emplace(std::piecewise_construct,
      std::forward_as_tuple(addr),
      std::forward_as_tuple(interval));

  sendHeartbeat(*ret.first);
}

void Heartbeat::disconnect(const std::string& addr)
{
  VLOG(1) << addr << " disconnected";
  conns_.erase(addr);
}

void Heartbeat::update(const std::string& addr)
{
  auto it = conns_.find(addr);
  if (it == conns_.end())
    return;

  it->second.lastRecvTime = hrc::now();
}

std::chrono::milliseconds Heartbeat::idle()
{
  const auto now = hrc::now();

  // Send any outstanding heartbeats.
  hrc::time_point closest_timeout = hrc::time_point::max();
  for (auto& kv : conns_)
  {
    if (kv.second.lastSendTime + kv.second.interval <= now)
      sendHeartbeat(kv);

    closest_timeout = std::min(closest_timeout,
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

  // TODO: implement heartbeat negotation
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      hrc::now() - closest_timeout);
}

void Heartbeat::sendHeartbeat(ConnMap::value_type& kv)
{
  VLOG(4) << "Sending HEARTBEAT to " << kv.first;

  heartbeatCallback_(kv.first);
  kv.second.lastSendTime = hrc::now();
}

} }
