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

class DelegatorTest : public testing::Test
{
public:
  DelegatorTest()
    : context_{1},
      worker_{context_, ZMQ_DEALER, "mockWorker", 0},
      requester_{context_, ZMQ_DEALER, "mockRequester", -1},
      running_{false}
  {
    DelegatorSettings settings = DelegatorSettings::Default(5555);
    settings.msPollRate = 100;
    settings.heartbeat.msPollRate = 100;
    settings.heartbeat.msTimeout = 1000;

    running_ = true;
    delFuture_ = stateline::startInThread<::stateline::comms::Delegator>(running_, std::ref(context_), std::ref(settings));

    worker_.setIdentifier("worker");
    worker_.connect("tcp://localhost:5555");

    requester_.setIdentifier();
    requester_.connect(DELEGATOR_SOCKET_ADDR.c_str());
  }

  ~DelegatorTest()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running_ = false;
    delFuture_.wait();
  }

  zmq::context_t context_;
  Socket worker_;
  Socket requester_;
  bool running_;
  std::future<bool> delFuture_;
};

TEST_F(DelegatorTest, canSendHelloToDelegator)
{
  worker_.send({ HELLO, { "A:B" }});
}

TEST_F(DelegatorTest, canSendAndReceiveSingleJobTypeMultipleTimes)
{
  worker_.send({ HELLO, { "A" }});

  // Send a job request
  requester_.send({{ "42" }, REQUEST, { "A", "Request 1" }});
  auto job1 = worker_.receive();
  EXPECT_EQ(Message(JOB, { "A", "0", "Request 1" }), job1);

  // Send another job request
  requester_.send({{ "36" }, REQUEST, { "A", "Request 2" }});
  auto job2 = worker_.receive();
  EXPECT_EQ(Message(JOB, { "A", "1", "Request 2" }), job2);
}

TEST_F(DelegatorTest, canSendAndReceiveMultipleJobTypes)
{
  worker_.send({ HELLO, { "A:B" }});

  // Send a job request with two job types
  requester_.send({{ "42" }, REQUEST, { "A:B", "Request" }});

  // Receive both of the requests
  auto job1 = worker_.receive();
  auto job2 = worker_.receive();

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
}

TEST_F(DelegatorTest, canReceiveResultForRequest)
{
  // Connect the worker
  worker_.send({ HELLO, { "A" }});

  // Send a job request
  requester_.send({{ "42" }, REQUEST, { "A", "Request" }});
  auto job = worker_.receive();
  EXPECT_EQ(Message(JOB, { "A", "0", "Request" }), job);

  // Send the job result
  worker_.send({RESULT, { "0", "Result" }});

  // Get the job result from the requester
  auto result = requester_.receive();
  EXPECT_EQ(Message({ "42" }, RESULT, { "Result" }), result);
}

TEST_F(DelegatorTest, multipleHelloMessages)
{
  worker_.send({ HELLO, { "A" }});
  worker_.send({ HELLO, { "B" }}); // Should be ignored
  worker_.send({ HELLO, { "C" }}); // Should be ignored
}

TEST_F(DelegatorTest, requesterSendsBeforeWorkerSaysHello)
{
  // Send a job request first
  requester_.send({{ "42" }, REQUEST, { "A", "Request 1" }});

  // Worker connects
  worker_.send({ HELLO, { "A" }});
  auto job1 = worker_.receive();
  EXPECT_EQ(Message(JOB, { "A", "0", "Request 1" }), job1);
}

TEST_F(DelegatorTest, resendsJobAfterWorkerFailure)
{
  // Send a job request
  requester_.send({{ "42" }, REQUEST, { "A", "Request" }});

  // Receive the job. This worker does not send heartbeats and will timeout eventually.
  worker_.send({ HELLO, { "A" }});
  worker_.receive();

  // Wait for this worker to time out
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // This new worker should get the job that was received by the dead worker.
  Socket newWorker{context_, ZMQ_DEALER, "mockNewWorker", 0};
  newWorker.setIdentifier("newWorker");
  newWorker.connect("tcp://localhost:5555");

  // Connect the new worker
  newWorker.send({ HELLO, { "A" }});

  // Wait for a job, this should unblock as soon as the heartbeat times out
  // and the previous worker considered dead.
  auto job = newWorker.receive();
  EXPECT_EQ(Message(JOB, { "A", "0", "Request" }), job);

  // Send the job result
  newWorker.send({RESULT, { "0", "Result" }});

  // Get the job result from the requester
  auto result = requester_.receive();
  EXPECT_EQ(Message({ "42" }, RESULT, { "Result" }), result);
}
