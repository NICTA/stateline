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

//! Settings for controlling heartbeat threads.
//!
struct HeartbeatSettings
{
  //! The number of milliseconds between each heartbeat.
  uint msRate;

  //! The rate at which the heartbeat sockets are polled.
  int msPollRate;

  //! The heartbeat timeout in milliseconds.
  uint msTimeout;

  static HeartbeatSettings WorkerDefault()
  {
    HeartbeatSettings settings;
    settings.msRate = 1000;
    settings.msPollRate = 500;
    settings.msTimeout = 3000;
    return settings;
  }

  static HeartbeatSettings DelegatorDefault()
  {
    HeartbeatSettings settings;
    settings.msRate = 1000;
    settings.msPollRate = 500;
    settings.msTimeout = 5000;
    return settings;
  }
};

//! Settings to control the behaviour of delegators.
//!
struct DelegatorSettings
{
  //! The rate at which the receive sockets are polled.
  int msPollRate;

  //! The port number that the delegator listens on.
  uint port;

  //! Settings for the heartbeat monitoring.
  HeartbeatSettings heartbeat;

  //! number of job types.
  uint nJobTypes;

  //! Default delegator settings
  static DelegatorSettings Default(uint port)
  {
    DelegatorSettings settings;
    settings.msPollRate = 10;
    settings.port = port;
    settings.heartbeat = HeartbeatSettings::DelegatorDefault();
    settings.nJobTypes = 1;
    return settings;
  }
};

//! Settings to control the behaviour of workers.
//!
struct WorkerSettings
{
  //! The address of the delegator to connect to.
  std::string networkAddress;

  //! The address of this worker which the minion connects to.
  std::string workerAddress;

  //! Settings for the heartbeat monitoring.
  HeartbeatSettings heartbeat;

  //! Default delegator settings
  WorkerSettings(std::string networkAddress, std::string workerAddress)
    : networkAddress{std::move(networkAddress)}
    , workerAddress{std::move(workerAddress)}
  {
    heartbeat = HeartbeatSettings::WorkerDefault();
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

  //! How long without heartbeats is considered times out.
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
