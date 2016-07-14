//! Contains the interface to the agent.
//!
//! \file comms/agent.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

#include <zmq.hpp>

#include "settings.hpp"
#include "endpoint.hpp"

namespace stateline { namespace comms {

//! An agent provides an interface between the delegator and worker.
//! It manages a set of workers running on the same host and forwards work
//! from the delegator to those workers.
//!
class Agent
{
public:
  //! Construct a new agent that can handle multiple types of jobs.
  //!
  //! \param settings The configuration object.
  //!
  Agent(zmq::context_t& ctx, const AgentSettings& settings);

  Agent(const Agent&) = delete;
  Agent& operator=(const Agent&) = delete;

  //! Poll the sockets once and handle any messages.
  void poll();

  //! Start the agent by polling indefinitely.
  void start(bool& running);

private:
  AgentSettings settings_;

  Socket worker_;
  Socket network_;
};

} }
