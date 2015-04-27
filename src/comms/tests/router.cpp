//!
//! \file comms/tests/router.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <gtest/gtest.h>

#include "comms/router.hpp"

using namespace stateline;
using namespace stateline::comms;

TEST(Router, canPollRouterWithNoSockets)
{
  zmq::context_t context{1};
  bool running = true;

  auto routerFuture = std::async(std::launch::async,
    [](bool& running)
    {
      SocketRouter router("testRouter", {});
      router.poll(100, running);
    }, std::ref(running));

  running = false;
  routerFuture.wait();
}

TEST(Router, routerPollCallsOnPollCallback)
{
  zmq::context_t context{1};
  bool running = true;

  Socket alpha{context, ZMQ_PAIR, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{context, ZMQ_PAIR, "beta"};
  beta.connect("inproc://alpha");

  auto routerFuture = std::async(std::launch::async,
    [](bool& running, Socket& alpha, Socket& beta)
    {
      SocketRouter router("testRouter", { &alpha, &beta });
      router.bindOnPoll([&]() { running = false; });
      router.poll(100, running);
    }, std::ref(running), std::ref(alpha), std::ref(beta));

  routerFuture.wait();
  EXPECT_FALSE(running);
}

TEST(Router, socketSendCallsBoundCallback)
{
  zmq::context_t context{1};
  bool running = true;

  Socket alpha{context, ZMQ_PAIR, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{context, ZMQ_PAIR, "beta"};
  beta.connect("inproc://alpha");

  auto routerFuture = std::async(std::launch::async,
    [](bool& running, Socket& alpha, Socket& beta)
    {
      bool alphaReceived = false;
      bool betaReceived = false;

      SocketRouter router("testRouter", { &alpha, &beta });

      router.bind(0, REQUEST, [&](const Message& m)
      {
        alphaReceived = true;
        EXPECT_EQ(REQUEST, m.subject);
        ASSERT_EQ(1U, m.data.size());
        EXPECT_EQ("request", m.data[0]);
      });

      router.bind(1, JOB, [&](const Message& m)
      {
        betaReceived = true;
        EXPECT_EQ(JOB, m.subject);
        ASSERT_EQ(1U, m.data.size());
        EXPECT_EQ("job", m.data[0]);
      });

      router.bindOnPoll([&]()
      {
        // Wait until both messages were received
        if (alphaReceived && betaReceived)
          running = false;
      });

      router.poll(100, running);

      ASSERT_TRUE(alphaReceived);
      ASSERT_TRUE(betaReceived);

    }, std::ref(running), std::ref(alpha), std::ref(beta));

  // Trigger the callbacks
  beta.send({REQUEST, { "request" }});
  alpha.send({JOB, { "job" }});

  routerFuture.wait();
}
