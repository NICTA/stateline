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

TEST_CASE("heartbeat can connect and disconnect", "[heartbeat]")
{
  Heartbeat hb;

  SECTION("defaults to no connections")
  {
    REQUIRE(hb.numConnections() == 0);
  }

  SECTION("connect updates connections")
  {
    hb.connect("foo", 1s);
    REQUIRE(hb.numConnections() == 1);

    SECTION("connecting same address updates connections")
    {
      hb.connect("foo", 2s); // TODO
      REQUIRE(hb.numConnections() == 1);
    }

    SECTION("disconnect updates connections")
    {
      hb.disconnect("foo");
      REQUIRE(hb.numConnections() == 0);

      SECTION("disconnecting same address does nothing")
      {
        hb.disconnect("foo");
        REQUIRE(hb.numConnections() == 0);
      }
    }
  }

  /*
  std::this_thread::sleep_for(500ms);
  hb.connect("bar", 1s);

  REQUIRE(hb.idle() < 500ms);*/
}
