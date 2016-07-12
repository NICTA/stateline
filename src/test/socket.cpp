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
using namespace std::chrono_literals;

TEST_CASE("can send messages over PAIR-PAIR SocketBases", "[socket_base]")
{
  zmq::context_t ctx{1};

  SECTION("constructor correctly initialises name")
  {
    SocketBase alpha{ctx, zmq::socket_type::pair, "alpha"};
    REQUIRE(alpha.name() == "alpha");

    SocketBase beta{ctx, zmq::socket_type::pair, "beta"};
    REQUIRE(beta.name() == "beta");

    SECTION("can bind and connect")
    {
      alpha.bind("inproc://alpha");
      beta.connect("inproc://alpha");

      SECTION("can send and receive")
      {
        for (int i = 0; i < 3; i++)
        {
          alpha.send("beta", std::to_string(i));

          const auto result = beta.recv();
          REQUIRE(result.first == "beta");
          REQUIRE(result.second == std::to_string(i));
        }
      }
    }
  }
}

TEST_CASE("can send messages over REQ-REP SocketBases", "[socket_base]")
{
  zmq::context_t ctx{1};

  SocketBase req{ctx, zmq::socket_type::req, "req"};
  SocketBase rep{ctx, zmq::socket_type::rep, "rep"};

  SECTION("can bind and connect")
  {
    rep.bind("inproc://test");
    req.connect("inproc://test");

    SECTION("can send and receive")
    {
      req.send("", "data");

      const auto result = rep.recv();
      REQUIRE(result.first == "");
      REQUIRE(result.second == "data");
    }
  }
}


TEST_CASE("can send messages over ROUTER-DEALER SocketBases", "[socket_base]")
{
  zmq::context_t ctx{1};

  SocketBase router{ctx, zmq::socket_type::router, "router"};
  router.bind("tcp://*:5174");

  SocketBase dealer{ctx, zmq::socket_type::dealer, "dealer"};
  dealer.setIdentity("dealer");
  dealer.connect("tcp://localhost:5174");

  for (int i = 0; i < 3; i++)
  {
    // When ROUTER receives from DEALER, there should be the address of the DEALER.
    dealer.send("", std::to_string(i));

    const auto result1 = router.recv();
    REQUIRE(result1.first == "dealer");
    REQUIRE(result1.second == std::to_string(i));

    // When DEALER receives from ROUTER, there should be no address.
    router.send("dealer", std::to_string(i));

    const auto result2 = dealer.recv();
    REQUIRE(result2.first == "");
    REQUIRE(result2.second == std::to_string(i));
  }
}

TEST_CASE("SocketBase keeps track of heartbeats", "[socket_base]")
{
  zmq::context_t ctx{1};

  SocketBase router{ctx, zmq::socket_type::router, "router"};
  router.bind("tcp://*:5174");

  SocketBase dealer{ctx, zmq::socket_type::dealer, "dealer"};
  dealer.setIdentity("dealer");
  dealer.connect("tcp://localhost:5174");

  SECTION("ignores incoming message if connection is not registered")
  {
    dealer.send("", "hi");
    router.recv();

    REQUIRE(router.heartbeats().numConnections() == 0);
  }

  SECTION("updates heartbeat manager when registering a new connection")
  {
    router.startHeartbeats("dealer", 1s);

    REQUIRE(router.heartbeats().numConnections() == 1);

    SECTION("updates last send time when sending")
    {
      const auto lastSendTime = router.heartbeats().lastSendTime("dealer");

      router.send("dealer", "hi");

      REQUIRE(router.heartbeats().lastSendTime("dealer") > lastSendTime);
    }

    SECTION("updates last recv time when receiving")
    {
      const auto lastRecvTime = router.heartbeats().lastRecvTime("dealer");

      dealer.send("", "hi");
      router.recv();

      REQUIRE(router.heartbeats().lastRecvTime("dealer") > lastRecvTime);
    }
  }
}

TEST_CASE("Can send messages over ROUTER-DEALER Sockets", "[socket]")
{
  zmq::context_t ctx{1};

  Socket router{ctx, zmq::socket_type::router, "router"};
  router.bind("tcp://*:5174");

  Socket dealer{ctx, zmq::socket_type::dealer, "dealer"};
  dealer.setIdentity("dealer");
  dealer.connect("tcp://localhost:5174");

  for (int i = 0; i < 3; i++)
  {
    // When ROUTER receives from DEALER, there should be the address of the DEALER.
    Message msg1{"", JOB, std::to_string(i)};
    dealer.send(msg1);

    const auto result1 = router.recv();
    REQUIRE(result1.address == "dealer");
    REQUIRE(result1.subject == JOB);
    REQUIRE(result1.data == std::to_string(i));

    // When DEALER receives from ROUTER, there should be no address.
    Message msg2{"dealer", RESULT, std::to_string(i)};
    router.send(msg2);

    const auto result2 = dealer.recv();
    REQUIRE(result2.address == "");
    REQUIRE(result2.subject == RESULT);
    REQUIRE(result2.data == std::to_string(i));
  }
}
