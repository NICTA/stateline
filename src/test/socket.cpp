//! Tests for socket.
//!
//! \file comms/tests/socket.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/socket.hpp"

using namespace stateline::comms;

TEST_CASE("Can create PAIR sockets", "[socket]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha", NO_LINGER};
  REQUIRE(alpha.name() == "alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta", NO_LINGER};
  REQUIRE(beta.name() == "beta");
}

TEST_CASE("Can bind and connect PAIR-PAIR", "[socket]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha", NO_LINGER};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta", NO_LINGER};
  beta.connect("inproc://alpha");
}

TEST_CASE("Can send messages over PAIR-PAIR", "[socket]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha", NO_LINGER};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta", NO_LINGER};
  beta.connect("inproc://alpha");

  for (int i = 0; i < 3; i++) {
    Message m{"beta", JOB, std::to_string(i)};
    alpha.send(m);

    auto result = beta.recv();
    REQUIRE(result.address == "beta");
    REQUIRE(result.subject == JOB);
    REQUIRE(result.data == std::to_string(i));
  }
}

TEST_CASE("Can bind and connect ROUTER-DEALER", "[socket]")
{
  zmq::context_t ctx{1};

  Socket router{ctx, zmq::socket_type::router, "router", NO_LINGER};
  router.bind("tcp://*:5000");

  Socket dealer{ctx, zmq::socket_type::dealer, "dealer", NO_LINGER};
  dealer.connect("tcp://localhost:5000");
}

TEST_CASE("Can send messages over ROUTER-DEALER", "[socket]")
{
  zmq::context_t ctx{1};

  Socket router{ctx, zmq::socket_type::router, "router", NO_LINGER};
  router.bind("tcp://*:5174");

  Socket dealer{ctx, zmq::socket_type::dealer, "dealer", NO_LINGER};
  dealer.setIdentity("dealer");
  dealer.connect("tcp://localhost:5174");

  for (int i = 0; i < 3; i++) {
    // When ROUTER receives from DEALER, there should be the address of the DEALER.
    Message m1{"", JOB, std::to_string(i)};
    dealer.send(m1);

    auto result1 = router.recv();
    REQUIRE(result1.address == "dealer");
    REQUIRE(result1.subject == JOB);
    REQUIRE(result1.data == std::to_string(i));

    // When DEALER receives from ROUTER, there should be no address.
    Message m2{"dealer", JOB, std::to_string(i)};
    router.send(m2);

    auto result2 = dealer.recv();
    REQUIRE(result2.address == "");
    REQUIRE(result2.subject == JOB);
    REQUIRE(result2.data == std::to_string(i));
  }
}
