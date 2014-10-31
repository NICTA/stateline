//!
//! \file comms/testrouter.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <chrono>
#include "gtest/gtest.h"
#include <thread>

#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/router.hpp"

#include <Eigen/Core>

using namespace stateline;
using namespace stateline::comms;

TEST(Routing, sendAndReceive)
{
  zmq::context_t context(1);
  zmq::socket_t alphaSocket(context, ZMQ_PAIR);
  std::unique_ptr<zmq::socket_t> alphaSocketRouter(new zmq::socket_t(context, ZMQ_PAIR));
  alphaSocket.bind("inproc://alpha");
  alphaSocketRouter->connect("inproc://alpha");

  SocketRouter router;

  router.add_socket(SocketID::ALPHA, alphaSocketRouter);

  Message hello(HELLO);
  Message problemspec(PROBLEMSPEC);

  send(alphaSocket, hello);
  auto rec = router.receive(SocketID::ALPHA);
  ASSERT_EQ(hello, rec);
  router.send(SocketID::ALPHA, problemspec);
  auto rec2 = receive(alphaSocket);
  ASSERT_EQ(problemspec, rec2);
}

TEST(Routing, polling)
{
  zmq::context_t context(1);
  zmq::socket_t alphaSocket(context, ZMQ_PAIR);
  std::unique_ptr<zmq::socket_t> alphaSocketRouter(new zmq::socket_t(context, ZMQ_PAIR));
  alphaSocket.bind("tcp://*:5555");
  alphaSocketRouter->connect("tcp://localhost:5555");

  SocketRouter router;
  router.add_socket(SocketID::ALPHA, alphaSocketRouter);

  Message hello(HELLO);
  bool okHELLO = false;

  auto onRcvHELLO = [&] (const Message&m)
  { okHELLO = m == hello;};
  router(SocketID::ALPHA).onRcvHELLO.connect(onRcvHELLO);

  bool running = true;
  router.start(10, running); //msPerPoll
  send(alphaSocket, hello);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  running = false;
  ASSERT_TRUE(okHELLO);
}

TEST(Routing, callbacks)
{
  zmq::context_t context(1);
  zmq::socket_t alphaSocket(context, ZMQ_PAIR);
  zmq::socket_t betaSocket(context, ZMQ_PAIR);
  std::unique_ptr<zmq::socket_t> alphaSocketRouter(new zmq::socket_t(context, ZMQ_PAIR));
  std::unique_ptr<zmq::socket_t> betaSocketRouter(new zmq::socket_t(context, ZMQ_PAIR));
  alphaSocket.bind("tcp://*:5555");
  betaSocket.bind("tcp://*:5556");
  alphaSocketRouter->connect("tcp://localhost:5555");
  betaSocketRouter->connect("tcp://localhost:5556");

  SocketRouter router;

  router.add_socket(SocketID::ALPHA, alphaSocketRouter);
  router.add_socket(SocketID::BETA, betaSocketRouter);

  Message hello(HELLO);
  Message heartbeat(HEARTBEAT);
  Message problemspec(PROBLEMSPEC);
  Message jobrequest(JOBREQUEST);
  Message job(JOB);
  Message jobswap(JOBSWAP);
  Message alldone(ALLDONE);
  Message goodbye(GOODBYE);

  auto fwdToBeta = [&] (const Message&m)
  { router.send(SocketID::BETA, m);};
  // Bind functionality to the router
  router(SocketID::ALPHA).onRcvHELLO.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvHEARTBEAT.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvPROBLEMSPEC.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvJOBREQUEST.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvJOB.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvJOBSWAP.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvALLDONE.connect(fwdToBeta);
  router(SocketID::ALPHA).onRcvGOODBYE.connect(fwdToBeta);

  bool running = true;
  router.start(10, running);//ms per poll
  send(alphaSocket, hello);
  Message rhello = receive(betaSocket);
  send(alphaSocket, heartbeat);
  Message rheartbeat = receive(betaSocket);
  send(alphaSocket, problemspec);
  Message rproblemspec = receive(betaSocket);
  send(alphaSocket, jobrequest);
  Message rjobrequest = receive(betaSocket);
  send(alphaSocket, job);
  Message rjob = receive(betaSocket);
  send(alphaSocket, jobswap);
  Message rjobswap = receive(betaSocket);
  send(alphaSocket, alldone);
  Message ralldone = receive(betaSocket);
  send(alphaSocket, goodbye);
  Message rgoodbye = receive(betaSocket);

  running = false;
  ASSERT_EQ(hello, rhello);
  ASSERT_EQ(heartbeat, rheartbeat);
  ASSERT_EQ(problemspec, rproblemspec);
  ASSERT_EQ(jobrequest, rjobrequest);
  ASSERT_EQ(job, rjob);
  ASSERT_EQ(jobswap, rjobswap);
  ASSERT_EQ(alldone, ralldone);
  ASSERT_EQ(goodbye, rgoodbye);
}
