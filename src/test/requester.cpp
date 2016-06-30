//! Tests for polling.
//!
//! \file comms/tests/poll.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/requester.hpp"
#include "comms/protobuf.hpp"

using namespace stateline::comms;

constexpr const char* DELEGATOR_ADDR = "inproc://test";

TEST_CASE("Can connect to inproc delegator", "[requester]")
{
  zmq::context_t ctx{1};

  Socket delegator{ctx, zmq::socket_type::router, "toRequester"};
  delegator.bind(DELEGATOR_ADDR);

  Requester req{ctx, DELEGATOR_ADDR};
}

TEST_CASE("Can submit to the delegator", "[requester]")
{
  zmq::context_t ctx{1};

  Socket delegator{ctx, zmq::socket_type::router, "toRequester"};
  delegator.bind(DELEGATOR_ADDR);

  Requester req{ctx, DELEGATOR_ADDR};
  req.submit(42, {1, 2}, {3, 4});

  const auto message = delegator.recv();
  REQUIRE(message.subject == BATCH_JOB);

  const auto batchJob = stringToProtobuf<messages::BatchJob>(message.data);

  REQUIRE(batchJob.id() == 42);
  REQUIRE(batchJob.job_type_size() == 2);
  REQUIRE(batchJob.job_type(0) == 1);
  REQUIRE(batchJob.job_type(1) == 2);
  REQUIRE(batchJob.data_size() == 2);
  REQUIRE(batchJob.data(0) == 3);
  REQUIRE(batchJob.data(1) == 4);
}

TEST_CASE("Can retrieve from the delegator", "[requester]")
{
  zmq::context_t ctx{1};

  Socket delegator{ctx, zmq::socket_type::router, "toRequester"};
  delegator.bind(DELEGATOR_ADDR);

  Requester req{ctx, DELEGATOR_ADDR};
  req.submit(42, {1, 2}, {3, 4}); // Submit so delegator can get its address

  const auto reqAddr = delegator.recv().address;

  messages::BatchResult batchResult;
  batchResult.set_id(42);
  batchResult.add_data(1);
  batchResult.add_data(2);

  delegator.send({reqAddr, BATCH_RESULT, protobufToString(batchResult)});

  const auto result = req.retrieve();

  REQUIRE(result.first == 42);
  REQUIRE(result.second.size() == 2);
  REQUIRE(result.second[0] == 1);
  REQUIRE(result.second[1] == 2);
}
