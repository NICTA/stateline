//!
//! \file comms/testdelegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "comms/settings.hpp"
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"
#include "comms/delegator.hpp"
#include "serial/comms.hpp"

#include <Eigen/Core>

namespace stateline
{
  namespace comms
  {

    // for testing
    std::string serialise(const std::string& s)
    {
      return s;
    }
    void unserialise(const std::string& s, std::string& t)
    {
      t = s;
    }

    enum class TestSocketID
    {
      ALPHA, BETA
    };

    class Delegation: public testing::Test
    {
    public:
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
        std::string perJobResults1 = "jobResult1";
        std::string perJobResults2 = "jobResult2";

        std::vector<uint> jobIds = { 2, 5 };
        std::vector<std::string> jobMsgs = { serialise(perJobSpec1), serialise(perJobSpec2) };
        std::vector<std::string> jobResMsgs = { serialise(perJobResults1), serialise(perJobResults2) };
        pDelegator.reset(new Delegator(serialise(globalSpec), jobIds, jobMsgs, jobResMsgs, settings));
        pDelegator->start();
      }

      ~Delegation()
      {

      }
    };

  TEST_F(Delegation, singleProblemSpec)
  {
    Delegator& delegator(*pDelegator);
    // Fake Worker
    zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
    auto workerID = randomSocketID();
    setSocketID(workerID, worker);
    worker.connect("tcp://localhost:5555");
    std::vector<uint> jobList =
    { 2};
    send(worker, Message(HELLO,
            { serialise(jobList)}));
    auto rep = receive(worker);
    send(worker, Message(HEARTBEAT));
    delegator.stop();
    send(worker, Message(HEARTBEAT));

    Message realRep(PROBLEMSPEC,
        { "globalSpec", "2", "jobSpec1", "jobResult1"});
    EXPECT_EQ(rep,realRep);
  }

  TEST_F(Delegation, multiProblemSpec)
  {
    Delegator& delegator(*pDelegator);
    // Fake Worker
    zmq::socket_t worker(delegator.zmqContext(), ZMQ_DEALER);
    auto workerID = randomSocketID();
    setSocketID(workerID, worker);
    worker.connect("tcp://localhost:5555");
    std::vector<uint> jobList =
    { 0,1,2,3,4,5};
    send(worker, Message(HELLO,
            { serialise(jobList)}));
    auto rep = receive(worker);
    send(worker, Message(HEARTBEAT));
    delegator.stop();
    send(worker, Message(HEARTBEAT));

    Message realRep(PROBLEMSPEC,
        { "globalSpec", "2", "jobSpec1", "jobResult1", "5", "jobSpec2", "jobResult2"});
    EXPECT_EQ(rep,realRep);
  }

  TEST_F(Delegation, job)
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
    std::vector<uint> jobList =
    { 0};
    worker.connect("tcp://localhost:5555");
    send(requester, Message(JOB,
            { serialise(0), "JOBDATA"}));
    send(worker, Message(HELLO,
            { serialise(jobList)}));
    receive(worker); // problemSpec
    send(worker, Message(JOBREQUEST,
            { serialise(0)}));
    auto rep = receive(worker); // job
    delegator.stop();

    Message realRep(
        { requesterID}, JOB,
        { serialise(0), "JOBDATA"});
    EXPECT_EQ(rep,realRep);
  }

  TEST_F(Delegation, job2)
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
    std::vector<uint> jobList =
    { 0,1};
    worker.connect("tcp://localhost:5555");
    send(requester, Message(JOB,
            { serialise(1), "JOBDATA"}));
    send(worker, Message(HELLO,
            { serialise(jobList)}));
    receive(worker); // problemSpec
    send(worker, Message(JOBREQUEST,
            { serialise(1)}));
    auto rep = receive(worker); // job
    delegator.stop();

    Message realRep(
        { requesterID}, JOB,
        { serialise(1), "JOBDATA"});
    EXPECT_EQ(rep,realRep);
  }

  TEST_F(Delegation, result)
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
    std::vector<uint> jobList =
    { 0};
    worker.connect("tcp://localhost:5555");
    send(requester, Message(JOB,
            { serialise(0), "JOBDATA"}));
    send(worker, Message(HELLO,
            { serialise(jobList)}));
    receive(worker); // problemSpec
    send(worker, Message(JOBREQUEST,
            { serialise(0)}));
    receive(worker); // job
    send(worker, Message(
            { requesterID, "minionAddress"}, JOBSWAP,
            { serialise(0), "Result", serialise(0)}));
    auto rep = receive(requester);
    delegator.stop();
    Message realRep(JOBSWAP,
        { serialise(0), "Result"});
    EXPECT_EQ(rep,realRep);
  }

  TEST_F(Delegation, newjob)
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
    send(requester, Message(JOB,
            { serialise(0), "JOBDATA"}));
    send(worker, Message(HELLO,
            { serialise(0)}));
    receive(worker); // problemSpec
    send(worker, Message(JOBREQUEST,
            { serialise(0)}));
    receive(worker); // job
    send(worker, Message(
            { requesterID, "minionAddress"}, JOBSWAP,
            { serialise(0), "Result", serialise(0)}));
    receive(requester);
    send(requester, Message(JOB,
            { serialise(0), "JOBDATA2"}));
    auto rep = receive(worker); // job

    delegator.stop();
    Message realRep(
        { requesterID, "minionAddress"}, JOB,
        { serialise(0), "JOBDATA2"});
    EXPECT_EQ(rep,realRep);
  }
}
 // namespace comms
}//namespace obsidian

