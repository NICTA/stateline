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
  bool disconnectCalled = false;

  Heartbeat hb;
  hb.bindDisconnect(
      [&disconnectCalled](const auto& addr, auto reason)
      {
        REQUIRE((reason == DisconnectReason::USER_REQUESTED));
        REQUIRE(addr == "foo");
        disconnectCalled = true;
      });

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

    SECTION("disconnect calls disconnect callback")
    {
      hb.disconnect("foo");
      REQUIRE(disconnectCalled == true);

      SECTION("update connections")
      {
        REQUIRE(hb.numConnections() == 0);
      }

      SECTION("disconnecting same address does nothing")
      {
        hb.disconnect("foo");
        REQUIRE(hb.numConnections() == 0);
      }
    }
  }
}

TEST_CASE("heartbeats timeout when no heartbeats are sent", "[heartbeat]")
{
  bool timeoutCalled = false;

  Heartbeat hb;
  hb.bindDisconnect(
      [&timeoutCalled](const auto& addr, auto reason)
      {
        REQUIRE(timeoutCalled == false);
        timeoutCalled = true;
        REQUIRE(addr == "foo");
        REQUIRE(reason == DisconnectReason::TIMEOUT);
      });

  hb.connect("foo", 1s);

  SECTION("calls timeout callback")
  {
    std::this_thread::sleep_for(1s);
    hb.idle();

    REQUIRE(timeoutCalled == true);

    SECTION("removes the connection")
    {
      REQUIRE(hb.numConnections() == 0);
    }
  }
}

TEST_CASE("heartbeats don't timeout when heartbeats are sent", "[heartbeat]")
{
  int heartbeatCalled = 0;

  Heartbeat hb;
  hb.bindHeartbeat(
      [&heartbeatCalled](const auto& addr)
      {
        REQUIRE(addr == "foo");
        heartbeatCalled++;
      });
  hb.bindDisconnect([](const auto&, auto) { REQUIRE(false); });

  hb.connect("foo", 1s);

  SECTION("calls heartbeat callback on idle")
  {
    std::this_thread::sleep_for(500ms);
    hb.updateLastRecvTime("foo");
    hb.idle();

    REQUIRE(heartbeatCalled == 1);

    SECTION("keeps the connection")
    {
      REQUIRE(hb.numConnections() == 1);
    }

    SECTION("calls heartbeat callback on second idle")
    {
      std::this_thread::sleep_for(500ms);
      hb.updateLastRecvTime("foo");
      hb.idle();

      REQUIRE(heartbeatCalled == 2);

      SECTION("keeps the connection")
      {
        REQUIRE(hb.numConnections() == 1);
      }

      SECTION("times out when we miss a heartbeat")
      {
        bool timeoutCalled = false;
        hb.bindDisconnect(
            [&timeoutCalled](const auto& addr, auto reason)
            {
              timeoutCalled = true;
              REQUIRE(addr == "foo");
              REQUIRE(reason == DisconnectReason::TIMEOUT);
            });

        std::this_thread::sleep_for(1s);
        hb.idle();

        SECTION("removes the connection")
        {
          REQUIRE(hb.numConnections() == 0);
        }
      }
    }
  }
}

// TODO: add test with multiple connections
