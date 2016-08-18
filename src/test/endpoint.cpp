//! Tests for endpoint.
//!
//! \file comms/tests/endpoint.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/endpoint.hpp"

using namespace stateline::comms;

namespace {

// TODO: put this into a separate file shared with other tests
struct TestEndpoint : Endpoint<TestEndpoint>
{
  TestEndpoint(Socket& socket) : Endpoint<TestEndpoint>{socket} { };

  void onHeartbeat(const Message& m) { handledHeartbeat = true; }

  bool handledHeartbeat{false};
};

}

TEST_CASE("endpoints handle messages correctly", "[endpoint]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint alphaEndpoint{alpha};

  SECTION("handles heartbeat message")
  {
    Message msg{"", HEARTBEAT, ""};
    beta.send(msg);

    alphaEndpoint.accept();
    REQUIRE(alphaEndpoint.handledHeartbeat);
  }
}
