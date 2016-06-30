//! Tests for heartbeat monitoring.
//!
//! \file comms/tests/poll.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/heartbeat.hpp"

#include <chrono>
#include <thread>

using namespace stateline::comms;
using namespace std::chrono_literals;

TEST_CASE("Heartbeat idle returns correct wait time", "[heartbeat]")
{
  Heartbeat hb;
  hb.connect("foo", 1s);
  std::this_thread::sleep_for(500ms);
  hb.connect("bar", 1s);

  REQUIRE(hb.idle() < 500ms);
}
