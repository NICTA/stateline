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
#include "comms/protobuf.hpp"

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
    REQUIRE(worker.send({ "", HELLO, "" }) == true); // TODO
    agent.poll();

    std::string agentAddress;
    {
      const auto msg = network.recv();
      agentAddress = msg.address;
      REQUIRE(msg.subject == HELLO);

      const auto hello = stringToProtobuf<messages::Hello>(msg.data);
      REQUIRE(hello.hb_timeout_secs() == 10);
    }

    SECTION("starts heartbeats after WELCOME from network")
    {
      messages::Welcome welcome;
      welcome.set_hb_timeout_secs(1);

      REQUIRE(network.send({ agentAddress, WELCOME, protobufToString(welcome) }) == true);
      agent.poll();

      // TODO: Should have received a heartbeat
      {
        //const auto msg = network.recv();
        //REQUIRE(msg.subject == HEARTBEAT);
      }

      SECTION("forwards JOB from network to worker")
      {
        {
          messages::Job job;
          job.set_id(1);
          job.set_job_type(2);
          job.add_data(1);

          REQUIRE(network.send({ agentAddress, JOB, protobufToString(job) }) == true);
        }

        agent.poll();

        // TODO: should have received a JOB

        SECTION("forwards RESULT from worker to network")
        {
          //REQUIRE(worker.send({ "", RESULT, "" }) == true);
          agent.poll();

          // TODO: should have received a RESULT
        }

        SECTION("queues up JOB if worker is busy")
        {
          {
            messages::Job job;
            job.set_id(2);
            job.set_job_type(2);
            job.add_data(1);

            REQUIRE(network.send({ agentAddress, JOB, protobufToString(job) }) == true);
          }

          agent.poll();

          // TODO should have queued

          SECTION("forwards RESULT from worker to network and sends queued JOB")
          {
            //REQUIRE(worker.send({ "", RESULT, "" }) == true);
            agent.poll();

            // TODO: should have received a RESULT
            // TODO: should send the queued job

            SECTION("forwards RESULT from worker to network")
            {
              // TODO: should have received a RESULT
            }
          }
        }
      }
    }
  }
}
