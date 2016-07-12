//!
//! \file comms/TEST_CASEs/router.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/endpoint.hpp"
#include "comms/router.hpp"

#include <chrono>

using namespace stateline::comms;
using namespace std::chrono_literals;

namespace {

struct TestEndpoint : Endpoint<TestEndpoint>
{
  TestEndpoint(Socket& socket)
    : Endpoint<TestEndpoint>{socket} { };

  void onHeartbeat(const Message& m) { handledHeartbeat = true; }

  bool handledHeartbeat{false};
};

}

TEST_CASE("can poll a router with no sockets", "[router]")
{
  zmq::context_t ctx{1};

  Router<> router{"test_router", std::tie()};

  bool idleCalled = false;
  router.poll([&idleCalled]() { idleCalled = true; });

  REQUIRE(idleCalled == true);
}

TEST_CASE("router poll calls the correct callbacks", "[router]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint alphaEndpoint{alpha};
  TestEndpoint betaEndpoint{beta};

  Router<TestEndpoint, TestEndpoint> router("test_router",
      std::tie(alphaEndpoint, betaEndpoint));

  SECTION("calls the correct callback when message is received")
  {
    bool idleCalled = false;

    Message msg{"", HEARTBEAT, ""};
    alpha.send(msg);
    router.poll([&idleCalled]() { idleCalled = true; });

    REQUIRE(betaEndpoint.handledHeartbeat == true);
    REQUIRE(idleCalled == true);
  }
}
