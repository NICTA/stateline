//! Tests for worker.
//!
//! \file comms/tests/worker.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/worker.hpp"
#include "comms/protobuf.hpp"

using namespace stateline::comms;

constexpr const char* DELEGATOR_ADDR = "tcp://*:5000";

WorkerSettings workerSettings()
{
  WorkerSettings settings;
  settings.networkAddress = DELEGATOR_ADDR;

  return settings;
}

TEST_CASE("Can connect to delegator and worker", "[worker]")
{
  zmq::context_t ctx{1};

  Socket delegator{ctx, zmq::socket_type::router, "toRequester"};
  delegator.bind(DELEGATOR_ADDR);

  Worker worker{ctx, settings};
}
