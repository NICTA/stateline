//! Tests for requester.
//!
//! \file comms/tests/requester.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/requester.hpp"
#include "comms/protocol.hpp"

using namespace stateline::comms;

constexpr const char* DELEGATOR_ADDR = "inproc://test";

TEST_CASE("can communicate with delegator", "[requester]")
{
  zmq::context_t ctx{1};

  Socket delegator{ctx, zmq::socket_type::router, "toRequester"};
  delegator.bind(DELEGATOR_ADDR);

  Requester req{ctx, DELEGATOR_ADDR};

  SECTION("sends batch job to the delegator on submit")
  {
    req.submit(42, {3, 4});

    const auto msg = delegator.recv();
    REQUIRE(msg.subject == BATCH_JOB);

    const auto batchJob = protocol::unserialise<protocol::BatchJob>(msg.data);
    REQUIRE(batchJob.id == 42);
    REQUIRE(batchJob.data.size() == 2);
    REQUIRE(batchJob.data[0] == 3);
    REQUIRE(batchJob.data[1] == 4);

    const auto reqAddress = msg.address;

    SECTION("obtains batch result on retrieve")
    {
      protocol::BatchResult batchResult;
      batchResult.id = 42;
      batchResult.data.push_back(1);
      batchResult.data.push_back(2);

      delegator.send({reqAddress, BATCH_RESULT, serialise(batchResult)});

      const auto result = req.retrieve();
      REQUIRE(result.first == 42);
      REQUIRE(result.second.size() == 2);
      REQUIRE(result.second[0] == 1);
      REQUIRE(result.second[1] == 2);
    }
  }
}
