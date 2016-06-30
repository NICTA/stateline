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

TEST_CASE("Can create an endpoint with a socket", "[endpoint]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint endpoint{alpha};

  Message msg{"", HEARTBEAT, ""};
  beta.send(msg);

  endpoint.accept();
  REQUIRE(endpoint.handledHeartbeat);
}

TEST_CASE("Endpoints have default implementations", "[endpoint]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  TestEndpoint endpoint{alpha};

  Message msg{"", HELLO, ""};
  beta.send(msg);

  endpoint.accept();
  REQUIRE(!endpoint.handledHeartbeat);
}
