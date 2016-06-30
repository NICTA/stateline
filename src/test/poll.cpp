//! Tests for polling.
//!
//! \file comms/tests/poll.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/endpoint.hpp"
#include "comms/poll.hpp"

#include <chrono>

using namespace stateline::comms;
using namespace std::chrono_literals;

namespace
{

struct TestEndpoint : Endpoint<TestEndpoint, Socket>
{
  TestEndpoint(Socket& s) : Endpoint<TestEndpoint, Socket>(s) { };

  void onHeartbeat(const Message& m)
  {
    handledHeartbeat = true;
  }

  bool handledHeartbeat{false};
};

}

TEST_CASE("Can poll with no endpoints", "[poll]")
{
  REQUIRE(poll(std::tie(), 100ms) == 0);
}

TEST_CASE("Polling inactive endpoints does not call any callbacks", "[poll]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint alphaEndpoint{alpha};
  TestEndpoint betaEndpoint{beta};

  REQUIRE(poll(std::tie(alphaEndpoint, betaEndpoint), 100ms) == 0);
  REQUIRE(!alphaEndpoint.handledHeartbeat);
  REQUIRE(!betaEndpoint.handledHeartbeat);
}

TEST_CASE("Polling handles single callback", "[poll]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint alphaEndpoint{alpha};
  TestEndpoint betaEndpoint{beta};

  alpha.send({"", HEARTBEAT, ""});

  REQUIRE(poll(std::tie(alphaEndpoint, betaEndpoint), 100ms) == 1);
  REQUIRE(!alphaEndpoint.handledHeartbeat);
  REQUIRE(betaEndpoint.handledHeartbeat);
}

TEST_CASE("Polling handles multiple callback", "[poll]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint alphaEndpoint{alpha};
  TestEndpoint betaEndpoint{beta};

  alpha.send({"", HEARTBEAT, ""});
  beta.send({"", HEARTBEAT, ""});

  REQUIRE(poll(std::tie(alphaEndpoint, betaEndpoint), 100ms) == 2);
  REQUIRE(alphaEndpoint.handledHeartbeat);
  REQUIRE(betaEndpoint.handledHeartbeat);
}
