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

#include <chrono>
#include <iostream>

using namespace stateline::comms;
using namespace std::chrono_literals;

TEST_CASE("delegator follows protocol", "[delegator]")
{
  zmq::context_t ctx{1};

  DelegatorSettings settings{"ipc://test_delegator", "ipc://test_network"};
  settings.heartbeatTimeout = 10s;
  settings.numJobTypes = 3;

  Socket network{ctx, zmq::socket_type::dealer, "network"};
  network.bind(settings.networkAddress);

  Delegator delegator{ctx, settings};

  Socket requester{ctx, zmq::socket_type::dealer, "requester"};
  requester.connect(settings.bindAddress);

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
        for (auto i = 0; i < settings.numJobTypes; i++)
        {
          const auto msg = network.recv();
          REQUIRE(msg.subject == JOB);

          jobs.push_back(protocol::unserialise<protocol::Job>(msg.data));
        }

        REQUIRE(jobs.size() == settings.numJobTypes);

        std::sort(jobs.begin(), jobs.end(), [](auto&& a, auto&& b) { return a.id < b.id; });

        for (auto i = 0; i < settings.numJobTypes; i++)
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
