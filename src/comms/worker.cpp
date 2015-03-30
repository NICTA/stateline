//!
//! Contains the implementation of the worker.
//!
//! \file comms/worker.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/worker.hpp"
#include "comms/serial.hpp"

#include <cstdlib>

namespace stateline
{
  namespace comms
  {
    void forwardToNetwork(const Message& m, SocketRouter& router)
    {
      VLOG(3) << "forwarding to delegator: " << m;
      router.send(SocketID::NETWORK, m);
    }

    void forwardToMinion(const Message& m, SocketRouter& router)
    {
      VLOG(3) << "forwarding to minion: " << m;
      router.send(SocketID::MINION, m);
    }

    void disconnectFromServer(const Message& m)
    {
      LOG(INFO)<< "Worker disconnecting from server";
      exit(EXIT_SUCCESS);
    }

    Worker::Worker(const std::vector<uint>& jobIDs, const WorkerSettings& settings)
      : running_(true)
    {
      // Setup sockets 
      context_ = new zmq::context_t(1);
      
      std::unique_ptr<zmq::socket_t> minion(new zmq::socket_t(*context_, ZMQ_ROUTER));
      std::unique_ptr<zmq::socket_t> heartbeat(new zmq::socket_t(*context_, ZMQ_PAIR));
      std::unique_ptr<zmq::socket_t> network(new zmq::socket_t(*context_, ZMQ_DEALER));
      minion->bind(WORKER_SOCKET_ADDR.c_str());
      heartbeat->bind(CLIENT_HB_SOCKET_ADDR.c_str());
      auto networkSocketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(networkSocketID, *(network));
      std::string address = "tcp://" + settings.address;
      LOG(INFO)<< "Worker connecting to " << address;
      network->connect(address.c_str());
      // Transfer sockets to the router
      router_.add_socket(SocketID::MINION, minion);
      router_.add_socket(SocketID::HEARTBEAT, heartbeat);
      router_.add_socket(SocketID::NETWORK, network);

      // Specify the Worker functionality
      auto onRcvMinion = [&] (const Message&m)
      { forwardToNetwork(m, this->router_);};
      auto onRcvNetwork = [&] (const Message&m)
      { forwardToMinion(m, this->router_);};

      auto fForwardToHB = [&] (const Message&m)
      { this->router_.send(SocketID::HEARTBEAT, m);};
      auto fForwardToNetwork = [&] (const Message&m)
      { this->router_.send(SocketID::NETWORK,m);};
      auto fDisconnect = [&] (const Message&m)
      { disconnectFromServer(m);};

      // Bind functionality to the router
      router_(SocketID::MINION).onRcvWORK.connect(onRcvMinion);
      router_(SocketID::NETWORK).onRcvWORK.connect(onRcvNetwork);

      router_(SocketID::NETWORK).onRcvHEARTBEAT.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvHELLO.connect(fForwardToHB);
      router_(SocketID::NETWORK).onRcvGOODBYE.connect(fForwardToHB);
      router_(SocketID::HEARTBEAT).onRcvHEARTBEAT.connect(fForwardToNetwork);
      router_(SocketID::HEARTBEAT).onRcvGOODBYE.connect(fDisconnect);

      // Initialise the connection
      router_.send(SocketID::NETWORK, Message(HELLO, { detail::serialise<std::uint32_t>(jobIDs) }));

      LOG(INFO)<< "Connection initialised";

      // Start the router and heartbeating
      router_.start(settings.msPollRate, running_);
      
      // Start the heartbeat system
      heartbeat_ = new ClientHeartbeat(*context_, settings.heartbeat);

    }

    Worker::~Worker()
    {
      running_ = false;
      delete context_;
      delete heartbeat_;
    }
  
  }
}
