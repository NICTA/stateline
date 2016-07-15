//!
//! Settings for communications system.
//!
//! \file comms/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <chrono>

#include "typedefs.hpp"

namespace stateline { namespace comms {

//! Settings to control the behaviour of delegators.
//!
struct DelegatorSettings
{
  //! The address of this delegator which the requester connects to.
  std::string bindAddress;

  //! The address of the agent to connect to.
  std::string networkAddress;

  //! How long without heartbeats is considered time out.
  std::chrono::seconds heartbeatTimeout;

  //! Number of job types per batch job.
  std::size_t numJobTypes;

  //! Construct default agent settings.
  //!
  DelegatorSettings(std::string bindAddress, std::string networkAddress)
    : bindAddress{std::move(bindAddress)}
    , networkAddress{std::move(networkAddress)}
  {
    heartbeatTimeout = std::chrono::seconds{15};
    numJobTypes = 1;
  }
};

//! Settings to control the behaviour of agents.
//!
struct AgentSettings
{
  //! The address of this agent which the worker connects to.
  std::string bindAddress;

  //! The address of the delegator to connect to.
  std::string networkAddress;

  //! How long without heartbeats is considered time out.
  std::chrono::seconds heartbeatTimeout;

  //! Construct default agent settings.
  //!
  AgentSettings(std::string bindAddress, std::string networkAddress)
    : bindAddress{std::move(bindAddress)}
    , networkAddress{std::move(networkAddress)}
  {
    heartbeatTimeout = std::chrono::seconds{15};
  }
};

} }
