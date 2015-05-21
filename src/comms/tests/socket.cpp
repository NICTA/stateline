//!
//! \file comms/tests/socket.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <gtest/gtest.h>

#include "comms/socket.hpp"

using namespace stateline::comms;

TEST(Socket, canCreatePairSocketsSharingContext)
{
  zmq::context_t context{1};

  Socket alpha{context, ZMQ_PAIR, "alpha"};
  EXPECT_EQ("alpha", alpha.name());

  Socket beta{context, ZMQ_PAIR, "beta"};
  EXPECT_EQ("beta", beta.name());
}

TEST(Socket, canBindAndConnectPairSockets)
{
  zmq::context_t context{1};

  Socket alpha{context, ZMQ_PAIR, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{context, ZMQ_PAIR, "beta"};
  beta.connect("inproc://alpha");
}

TEST(Socket, canSendMessagesOverPairSockets)
{
  zmq::context_t context{1};

  Socket alpha{context, ZMQ_PAIR, "alpha"};
  alpha.bind("inproc://alpha");

  Socket beta{context, ZMQ_PAIR, "beta"};
  beta.connect("inproc://alpha");

  for (int i = 0; i < 10; i++) {
    Message m{JOB, { std::to_string(i) }};
    alpha.send(m);

    auto result = beta.receive();
    EXPECT_EQ(JOB, result.subject);
    ASSERT_EQ(1U, result.data.size());
    EXPECT_EQ(std::to_string(i), result.data[0]);
  }
}

TEST(Socket, sendFailureThrowsExceptionByDefault)
{
  zmq::context_t context{1};
  Socket alpha{context, ZMQ_REP, "alpha"};
  Message m{JOB, { "failure!" }};
  ASSERT_THROW(alpha.send(m), std::exception);
}

TEST(Socket, sendFailureCallsCustomFallback)
{
  zmq::context_t context{1};
  Socket alpha{context, ZMQ_REP, "alpha"};

  bool calledFallback = false;
  alpha.setFallback([&](const Message &m)
  {
    calledFallback = true;
    EXPECT_EQ(JOB, m.subject);
    ASSERT_EQ(1U, m.data.size());
    EXPECT_EQ("failure!", m.data[0]);
  });

  Message m{JOB, { "failure!" }};
  alpha.send(m);
  ASSERT_TRUE(calledFallback);
}
