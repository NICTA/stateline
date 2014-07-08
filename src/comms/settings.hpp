//!
//! Settings for communications system.
//!
//! \file comms/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

namespace stateline
{
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
      HeartbeatSettings settings = {};
      settings.msRate = 1000;
      settings.msPollRate = 500;
      settings.msTimeout = 3000;
      return settings;
    }

    static HeartbeatSettings DelegatorDefault()
    {
      HeartbeatSettings settings = {};
      settings.msRate = 1000;
      settings.msPollRate = 500;
      settings.msTimeout = 10000;
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

    //! Default delegator settings
    static DelegatorSettings Default(uint port)
    {
      DelegatorSettings settings = {};
      settings.msPollRate = 100;
      settings.port = port;
      settings.heartbeat = HeartbeatSettings::DelegatorDefault();
      return settings;
    }
  };

  //! Settings to control the behaviour of workers.
  //!
  struct WorkerSettings
  {
    //! The rate at which the receive sockets are polled.
    int msPollRate;

    //! The address of the delegator to connect to.
    std::string address;

    //! Settings for the heartbeat monitoring.
    HeartbeatSettings heartbeat;

    //! Default delegator settings
    static WorkerSettings Default(const std::string &address)
    {
      WorkerSettings settings = {};
      settings.msPollRate = 100;
      settings.address = address;
      settings.heartbeat = HeartbeatSettings::WorkerDefault();
      return settings;
    }
  };
}

