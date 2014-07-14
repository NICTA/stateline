//!
//! \file comms/testdelegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <gtest/gtest.h>

#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/delegator.hpp"
#include "comms/serial.hpp"

#include <Eigen/Core>

using namespace stateline;
using namespace stateline::comms;
using namespace stateline::comms::detail;

class Delegation: public testing::Test
{
  protected:
    std::unique_ptr<Delegator> pDelegator;

    Delegation()
    {
      DelegatorSettings settings;
      settings.port = 5555;
      settings.msPollRate = 100;
      settings.heartbeat.msRate = 1000;
      settings.heartbeat.msPollRate = 500;
      settings.heartbeat.msTimeout = 3000;

      std::string globalSpec = "globalSpec";
      std::string perJobSpec1 = "jobSpec1";
      std::string perJobSpec2 = "jobSpec2";

      std::map<JobID, std::string> jobMsgs =
      {
        { 2, perJobSpec1 }, 
        { 5, perJobSpec2 }
      };

      pDelegator.reset(new Delegator(globalSpec, jobMsgs, settings));
      pDelegator->start();
    }

    ~Delegation()
    {

    }
};

TEST_F(Delegation, canSendAndReceiveSingleProblemSpec)
{
  Delegator& delegator(*pDelegator);

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");
  
  // Send a job list to the delegator
  std::vector<uint> jobList = { 2 };
  send(worker, Message(HELLO, { serialise<std::uint32_t>(jobList) }));
  auto rep = receive(worker);
  send(worker, Message(HEARTBEAT));
  delegator.stop();
  send(worker, Message(HEARTBEAT));

  Message expected(PROBLEMSPEC, { "globalSpec", "2", "jobSpec1" });
  EXPECT_EQ(expected, rep);
}

TEST_F(Delegation, canSendAndReceiveMultiProblemSpec)
{
  Delegator& delegator(*pDelegator);

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");

  // Send multiple jobs in one list (only the specs for 2 and 5 are valid)
  std::vector<uint> jobList = { 0, 1, 2, 3, 4, 5 };
  send(worker, Message(HELLO, { serialise<std::uint32_t>(jobList) }));
  auto rep = receive(worker);
  send(worker, Message(HEARTBEAT));
  delegator.stop();
  send(worker, Message(HEARTBEAT));

  Message expected(PROBLEMSPEC,
      { "globalSpec", "2", "jobSpec1", "5", "jobSpec2" });
  EXPECT_EQ(expected, rep);
}

TEST_F(Delegation, canSendAndReceiveJobRequest)
{
  Delegator& delegator(*pDelegator);

  // Fake requester 
  zmq::socket_t requester(delegator.zmqContext(), ZMQ_DEALER);
  auto requesterID = randomSocketID();
  setSocketID(requesterID, requester);
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");
  
  // Send job data to worker
  send(requester, Message(JOB, { serialise<std::uint32_t>(2), "JOBDATA" }));

  // Send job list to requester
  std::vector<uint> jobList = { 2 };
  send(worker, Message(HELLO, { serialise<std::uint32_t>(jobList) }));
  receive(worker); // problemSpec

  // Send job request to requester
  send(worker, Message(JOBREQUEST, { serialise<std::uint32_t>(2) }));
  auto rep = receive(worker); // job
  delegator.stop();

  Message expected({ requesterID}, JOB, { serialise<std::uint32_t>(2), "JOBDATA" });
  EXPECT_EQ(expected, rep);
}

TEST_F(Delegation, canSendAndReceiveJobRequestWithMultipleJobTypes)
{
  Delegator& delegator(*pDelegator);

  // Fake requester 
  zmq::socket_t requester(delegator.zmqContext(), ZMQ_DEALER);
  auto requesterID = randomSocketID();
  setSocketID(requesterID, requester);
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");

  // Send job list to requester
  std::vector<uint> jobList = { 2, 5 };
  send(requester, Message(JOB, { serialise<std::uint32_t>(2), "JOBDATA" }));
  send(worker, Message(HELLO, { serialise<std::uint32_t>(jobList) }));
  receive(worker); // problemSpec

  // Send job request to requester
  send(worker, Message(JOBREQUEST, { serialise<std::uint32_t>(2) }));
  auto rep = receive(worker); // job
  delegator.stop();

  Message expected({ requesterID}, JOB, { serialise<std::uint32_t>(2), "JOBDATA"});
  EXPECT_EQ(expected, rep);
}

TEST_F(Delegation, canSendResultsToRequester)
{
  Delegator& delegator(*pDelegator);

  // Fake requester 
  zmq::socket_t requester(delegator.zmqContext(), ZMQ_DEALER);
  auto requesterID = randomSocketID();
  setSocketID(requesterID, requester);
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");

  // Send job list to worker
  std::vector<uint> jobList = { 2 };
  send(requester, Message(JOB, { serialise<std::uint32_t>(2), "JOBDATA" }));
  send(worker, Message(HELLO, { serialise<std::uint32_t>(jobList) }));
  receive(worker); // problemSpec

  // Send a job request
  send(worker, Message(JOBREQUEST, { serialise<std::uint32_t>(2)}));

  // Receive a job and return a result
  receive(worker); // job
  send(worker, Message(
        { requesterID, "minionAddress"}, JOBSWAP,
        { serialise<std::uint32_t>(2), "Result", serialise<std::uint32_t>(2) }));

  // Retrieve the result
  auto rep = receive(requester);
  delegator.stop();

  Message expected(JOBSWAP, { serialise<std::uint32_t>(2), "Result" });
  EXPECT_EQ(expected, rep);
}

TEST_F(Delegation, canRequestJobAfterJob)
{
  Delegator& delegator(*pDelegator);

  // Fake requester 
  zmq::socket_t requester(delegator.zmqContext(), ZMQ_DEALER);
  auto requesterID = randomSocketID();
  setSocketID(requesterID, requester);
  requester.connect(DELEGATOR_SOCKET_ADDR.c_str());

  // Fake Worker
  zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
  auto workerID = randomSocketID();
  setSocketID(workerID, worker);
  worker.connect("tcp://localhost:5555");

  // Dialog
  send(requester, Message(JOB, { serialise<std::uint32_t>(2), "JOBDATA" })); 
  send(worker, Message(HELLO, { serialise<std::uint32_t>(2) }));
  receive(worker); // problemSpec

  send(worker, Message(JOBREQUEST, { serialise<std::uint32_t>(2) }));
  receive(worker); // job
  send(worker, Message(
        { requesterID, "minionAddress"}, JOBSWAP,
        { serialise<std::uint32_t>(2), "Result", serialise<std::uint32_t>(2) }));
  receive(requester);
  send(requester, Message(JOB, { serialise<std::uint32_t>(2), "JOBDATA2" }));
  auto rep = receive(worker); // job

  delegator.stop();
  Message expected(
      { requesterID, "minionAddress"}, JOB,
      { serialise<std::uint32_t>(2), "JOBDATA2" });
  EXPECT_EQ(expected, rep);
}
