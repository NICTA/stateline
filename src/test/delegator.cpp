//! Tests for delegator.
//!
//! \file comms/tests/delegator.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/delegator.hpp"
#include "comms/protocol.hpp"

#include "testsocket.hpp"

#include <chrono>
#include <iostream>

using namespace stateline::comms;
using namespace stateline::test;
using namespace std::chrono_literals;

TEST_CASE("delegator follows protocol", "[delegator]")
{
  zmq::context_t ctx{1};

  DelegatorSettings settings{"ipc://test_delegator", "ipc://test_network"};
  settings.heartbeatTimeout = 10s;
  settings.numJobTypes = 3;

  TestSocket network{ctx, zmq::socket_type::dealer, "network"};
  network.connect(settings.networkAddress);

  Delegator delegator{ctx, settings};

  Socket requester{ctx, zmq::socket_type::dealer, "requester"};
  requester.connect(settings.requesterAddress);

  SECTION("responds to HELLO from worker with WELCOME")
  {
    {
      protocol::Hello hello;
      hello.jobTypesRange = std::make_pair(1, 3);
      hello.hbTimeoutSecs = 20;

      REQUIRE(network.send({"", HELLO, serialise(hello)}) == true);
    }

    delegator.poll();

    {
      const auto msg = network.recv();
      REQUIRE(msg.subject == WELCOME);

      const auto welcome = protocol::unserialise<protocol::Welcome>(msg.data);
      REQUIRE(welcome.hbTimeoutSecs == 20);
    }

    SECTION("sends jobs to worker when BATCH JOB is received")
    {
      {
        protocol::BatchJob batchJob;
        batchJob.id = 1;
        batchJob.data.push_back(1);
        batchJob.data.push_back(2);
        batchJob.data.push_back(3);

        REQUIRE(requester.send({"", BATCH_JOB, serialise(batchJob)}) == true);
      }

      delegator.poll();

      {
        std::vector<protocol::Job> jobs;
        for (std::size_t i = 0; i < settings.numJobTypes; i++)
        {
          const auto msg = network.recv();
          REQUIRE(msg.subject == JOB);

          jobs.push_back(protocol::unserialise<protocol::Job>(msg.data));
        }

        REQUIRE(jobs.size() == settings.numJobTypes);

        std::sort(jobs.begin(), jobs.end(), [](auto&& a, auto&& b) { return a.id < b.id; });

        for (std::size_t i = 0; i < settings.numJobTypes; i++)
        {
          REQUIRE(jobs[i].id == i + 1);
          REQUIRE(jobs[i].data.size() == 3);
          REQUIRE(jobs[i].data[0] == 1);
          REQUIRE(jobs[i].data[1] == 2);
          REQUIRE(jobs[i].data[2] == 3);
        }
      }

      SECTION("sends BATCH RESULT when all the results have been collected")
      {
        for (std::size_t i = 0; i < settings.numJobTypes; i++)
        {
          protocol::Result result;
          result.id = i + 1;
          result.data = i;

          REQUIRE(network.send({"", RESULT, serialise(result)}) == true);

          // Got to poll once per result since it's from the same socket
          delegator.poll();
        }

        {
          const auto msg = requester.recv();
          REQUIRE(msg.subject == BATCH_RESULT);

          const auto batchResult = protocol::unserialise<protocol::BatchResult>(msg.data);
          REQUIRE(batchResult.id == 1);
          REQUIRE(batchResult.data.size() == 3);
          REQUIRE(batchResult.data[0] == 0.0);
          REQUIRE(batchResult.data[1] == 1.0);
          REQUIRE(batchResult.data[2] == 2.0);
        }
      }
    }
  }
}

TEST_CASE("delegator delegates jobs fairly", "[delegator]")
{
  zmq::context_t ctx{1};

  DelegatorSettings settings{"ipc://test_delegator", "ipc://test_network"};
  settings.heartbeatTimeout = 10s;
  settings.numJobTypes = 2;

  TestSocket network1{ctx, zmq::socket_type::dealer, "network"};
  network1.connect(settings.networkAddress);

  TestSocket network2{ctx, zmq::socket_type::dealer, "network"};
  network2.connect(settings.networkAddress);

  Delegator delegator{ctx, settings};

  Socket requester{ctx, zmq::socket_type::dealer, "requester"};
  requester.connect(settings.requesterAddress);

  SECTION("responds to HELLOs from workers")
  {
    {
      protocol::Hello hello;
      hello.jobTypesRange = std::make_pair(1, 2);
      hello.hbTimeoutSecs = 20;
      REQUIRE(network1.send({"", HELLO, serialise(hello)}) == true);
      REQUIRE(network2.send({"", HELLO, serialise(hello)}) == true);
    }

    // Poll twice because the messages are from the same socket
    delegator.poll();
    delegator.poll();

    {
      const auto msg1 = network1.recv();
      REQUIRE(msg1.subject == WELCOME);

      const auto msg2 = network2.recv();
      REQUIRE(msg2.subject == WELCOME);
    }

    SECTION("sends jobs to worker when BATCH JOB is received")
    {
      {
        protocol::BatchJob batchJob;
        batchJob.id = 1;
        batchJob.data.push_back(1); // Should be sent to worker1
        batchJob.data.push_back(2); // Should be sent to worker1 again
        batchJob.data.push_back(3); // Should be sent to worker2

        REQUIRE(requester.send({"", BATCH_JOB, serialise(batchJob)}) == true);
      }

      delegator.poll();

      {
        const auto msg1 = network1.recv();
        REQUIRE(msg1.subject == JOB);

        const auto msg2 = network2.recv();
        REQUIRE(msg2.subject == JOB);

        const auto job1 = protocol::unserialise<protocol::Job>(msg1.data);
        const auto job2 = protocol::unserialise<protocol::Job>(msg2.data);

        // The two jobs should be split between the two workers
        if (job1.id == 1)
          REQUIRE(job2.id == 2);
        else
          REQUIRE(job2.id == 1);
      }
    }
  }
}
