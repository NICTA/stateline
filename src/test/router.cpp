//!
//! \file comms/TEST_CASEs/router.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/router.hpp"

#include <chrono>

using namespace stateline::comms;
using namespace std::chrono_literals;

TEST_CASE("Can poll a router with no sockets", "[router]")
{
  zmq::context_t ctx{1};

  Router<> router{"test_router", std::tuple<>{}};
  router.pollStep(100ms);
}

TEST_CASE("Poll calls onPoll callback", "[router]")
{
  zmq::context_t ctx{1};

  bool called = false;
  Router<> router("test_router", std::tuple<>{});
  router.bindOnPoll([&called]() { called = true; });
  router.pollStep(100ms);

  REQUIRE(called == true);
}

TEST_CASE("Router calls bound callbacks", "[router]")
{
  zmq::context_t ctx{1};

  Socket alpha{ctx, zmq::socket_type::pair, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{ctx, zmq::socket_type::pair, "beta"};
  beta.connect("inproc://alpha");

  auto alphaReceived = false;
  auto betaReceived = false;

  Router<Socket, Socket> router{"test_router", std::tie(alpha, beta)};
  router.bind<0, RESULT>([&alphaReceived](const Message& m)
  {
    REQUIRE(m.subject == RESULT);
    REQUIRE(m.data == "result");
    alphaReceived = true;
  });

  router.bind<1, JOB>([&betaReceived](const Message& m)
  {
    REQUIRE(m.subject == JOB);
    REQUIRE(m.data == "job");
    betaReceived = true;
  });

  // Trigger the beta callback
  alpha.send({"", JOB, "job"});
  router.pollStep(100ms);
  REQUIRE(betaReceived);

  // Trigger the alpha callback
  beta.send({"", RESULT, "result"});
  router.pollStep(100ms);
  REQUIRE(alphaReceived);
}
