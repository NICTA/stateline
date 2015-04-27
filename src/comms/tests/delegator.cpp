//!
//! \file comms/tests/message.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <gtest/gtest.h>

#include "comms/delegator.hpp"

#include "comms/socket.hpp"
#include "comms/thread.hpp"

using namespace stateline::comms;

// Custom print for message to make it easier to debug
namespace stateline
{
  namespace comms
  {
    void PrintTo(const Message& m, std::ostream* os)
    {
      *os << "|";
      for (const auto& addr : m.address)
      {
        *os << addr << "|";
      }
      *os << m.subject << "|";
      for (const auto& data : m.data)
      {
        *os << data << "|";
      }
    }
  }
}

TEST(Delegator, canSendHelloToDelegator)
{
  zmq::context_t context{1};
  bool running = true;

  DelegatorSettings settings = DelegatorSettings::Default(5555);
  settings.msPollRate = 100;
  settings.heartbeat.msPollRate = 100;
  auto delegatorFuture = stateline::startInThread<Delegator>(running, std::ref(context), std::ref(settings));

  Socket worker{context, ZMQ_DEALER, "mockWorker"};
  worker.setIdentifier("worker");
  worker.connect("tcp://localhost:5555");

  Message hello{HELLO, { "A:B"}};
  worker.send(hello);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  running = false;
  delegatorFuture.wait();
}

TEST(Delegator, canSendAndReceiveSingleJobTypeMultipleTimes)
{
  zmq::context_t context{1};
  bool running = true;

  DelegatorSettings settings = DelegatorSettings::Default(5555);
  settings.msPollRate = 100;
  settings.heartbeat.msPollRate = 100;
  auto delegatorFuture = stateline::startInThread<Delegator>(running, std::ref(context), std::ref(settings));

  Socket worker{context, ZMQ_DEALER, "mockWorker"};
  worker.setIdentifier();
  worker.connect("tcp://localhost:5555");

  Socket requester{context, ZMQ_DEALER, "mockRequester"};
  requester.setIdentifier();
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Connect the worker
  Message hello{HELLO, { "A" }};
  worker.send(hello);

  // Send a job request
  requester.send({{ "42" }, REQUEST, { "A", "Request 1" }});

  // Receive the request
  auto job1 = worker.receive();
  EXPECT_EQ(Message(JOB, { "A", "0", "Request 1" }), job1);

  // Send another job request
  requester.send({{ "36" }, REQUEST, { "A", "Request 2" }});

  // Receive the request
  auto job2 = worker.receive();
  EXPECT_EQ(Message(JOB, { "A", "1", "Request 2" }), job2);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  running = false;
  delegatorFuture.wait();
}

TEST(Delegator, canSendAndReceiveMultipleJobTypes)
{
  zmq::context_t context{1};
  bool running = true;

  DelegatorSettings settings = DelegatorSettings::Default(5555);
  settings.msPollRate = 100;
  settings.heartbeat.msPollRate = 100;
  auto delegatorFuture = stateline::startInThread<Delegator>(running, std::ref(context), std::ref(settings));

  Socket worker{context, ZMQ_DEALER, "mockWorker"};
  worker.setIdentifier();
  worker.connect("tcp://localhost:5555");

  Socket requester{context, ZMQ_DEALER, "mockRequester"};
  requester.setIdentifier();
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Connect the worker
  Message hello{HELLO, { "A:B" }};
  worker.send(hello);

  // Send a job request
  requester.send({{ "42" }, REQUEST, { "A:B", "Request" }});

  // Receive both of the requests
  auto job1 = worker.receive();
  auto job2 = worker.receive();

  ASSERT_EQ(3U, job1.data.size());
  ASSERT_EQ(3U, job2.data.size());

  if (job1.data[0] == "A")
  {
    EXPECT_EQ(Message(JOB, { "A", "0", "Request" }), job1);
    EXPECT_EQ(Message(JOB, { "B", "1", "Request" }), job2);
  }
  else
  {
    EXPECT_EQ(Message(JOB, { "B", "0", "Request" }), job1);
    EXPECT_EQ(Message(JOB, { "A", "1", "Request" }), job2);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  running = false;
  delegatorFuture.wait();
}
