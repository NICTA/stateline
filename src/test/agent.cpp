//! Tests for agent.
//!
//! \file comms/tests/agent.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/agent.hpp"
#include "comms/protocol.hpp"

#include <chrono>

using namespace stateline::comms;
using namespace std::chrono_literals;

TEST_CASE("agent can connect to network and worker", "[agent]")
{
  zmq::context_t ctx{1};

  AgentSettings settings{"ipc://test_agent", "ipc://test_network"};
  settings.heartbeatTimeout = 10s;

  Socket network{ctx, zmq::socket_type::router, "network"};
  network.bind(settings.networkAddress);

  Agent agent{ctx, settings};

  Socket worker{ctx, zmq::socket_type::req, "worker"};
  worker.connect(settings.bindAddress);

  SECTION("forwards HELLO from worker to network")
  {
    {
      protocol::Hello hello;
      hello.jobTypesRange = std::make_pair(1, 3);
      hello.hbTimeoutSecs = 10;

      REQUIRE(worker.send({"", HELLO, serialise(hello)}) == true);
    }

    agent.poll();

    std::string agentAddress;
    {
      const auto msg = network.recv();
      agentAddress = msg.address;
      REQUIRE(msg.subject == HELLO);

      const auto hello = protocol::unserialise<protocol::Hello>(msg.data);
      REQUIRE(hello.jobTypesRange.first == 1);
      REQUIRE(hello.jobTypesRange.second == 3);
      REQUIRE(hello.hbTimeoutSecs == 10);
    }

    SECTION("starts heartbeats after WELCOME from network")
    {
      protocol::Welcome welcome;
      welcome.hbTimeoutSecs = 10;

      REQUIRE(network.send({ agentAddress, WELCOME, serialise(welcome) }) == true);
      agent.poll();

      // TODO:

      SECTION("forwards JOB from network to worker")
      {
        {
          protocol::Job job;
          job.id = 1;
          job.type = 2;
          job.data.push_back(1);
          job.data.push_back(2);
          job.data.push_back(3);

          REQUIRE(network.send({ agentAddress, JOB, serialise(job) }) == true);
        }

        agent.poll();

        {
          const auto msg = worker.recv();
          REQUIRE(msg.subject == JOB);

          const auto job = protocol::unserialise<protocol::Job>(msg.data);

          REQUIRE(job.id == 1);
          REQUIRE(job.type == 2);
          REQUIRE(job.data.size() == 3);
          REQUIRE(job.data[0] == 1);
          REQUIRE(job.data[1] == 2);
          REQUIRE(job.data[2] == 3);
        }

        SECTION("forwards RESULT from worker to network")
        {
          protocol::Result result;
          result.id = 1;
          result.data = 4;

          REQUIRE(worker.send({ "", RESULT, serialise(result) }) == true);
          agent.poll();

          {
            const auto msg = network.recv();
            REQUIRE(msg.subject == RESULT);

            const auto result = protocol::unserialise<protocol::Result>(msg.data);
            REQUIRE(result.id == 1);
            REQUIRE(result.data == 4);
          }
        }

        SECTION("queues up JOB if worker is busy")
        {
          {
            protocol::Job job;
            job.id = 2;
            job.type = 2;
            job.data.push_back(1);

            REQUIRE(network.send({ agentAddress, JOB, serialise(job) }) == true);
          }

          agent.poll();

          SECTION("forwards RESULT from worker to network and sends queued JOB")
          {
            {
              protocol::Result result;
              result.id = 1;
              result.data = 4;

              REQUIRE(worker.send({ "", RESULT, serialise(result) }) == true);
            }

            agent.poll();

            {
              const auto msg = network.recv();
              REQUIRE(msg.subject == RESULT);

              const auto result = protocol::unserialise<protocol::Result>(msg.data);
              REQUIRE(result.id == 1);
              REQUIRE(result.data == 4);
            }

            {
              const auto msg = worker.recv();
              REQUIRE(msg.subject == JOB);

              const auto job = protocol::unserialise<protocol::Job>(msg.data);

              REQUIRE(job.id == 2);
              REQUIRE(job.type == 2);
              REQUIRE(job.data.size() == 1);
              REQUIRE(job.data[0] == 1);
            }

            SECTION("forwards RESULT from worker to network")
            {
              {
                protocol::Result result;
                result.id = 2;
                result.data = 5;

                REQUIRE(worker.send({ "", RESULT, serialise(result) }) == true);
              }

              agent.poll();

              {
                const auto msg = network.recv();
                REQUIRE(msg.subject == RESULT);

                const auto result = protocol::unserialise<protocol::Result>(msg.data);
                REQUIRE(result.id == 2);
                REQUIRE(result.data == 5);
              }
            }
          }
        }
      }
    }
  }
}
