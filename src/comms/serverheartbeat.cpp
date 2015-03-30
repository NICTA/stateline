//!
//! Implementation of heartbeat routers that deal with server heartbeating sending and receiving.
//!
//! \file comms/serverheartbeat.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/serverheartbeat.hpp"
#include <stdexcept>
#include <glog/logging.h>

namespace stateline
{
  namespace comms
  {
    //! Add a new client to be monitored for heartbeating.
    //!
    //! \param m The HELLO message from the client.
    //! \param clients List of clients being monitored.
    //! \param lastHeartbeats Map containing when the last heartbeat arrived from each client.
    //!
    void insertClient(const Message& m, HBClients& clients, HBMap& lastHeartbeats);

    //! Remove a client from being monitored for heartbeating.
    //!
    //! \param m The GOODBYE message from the client.
    //! \param clients List of clients being monitored.
    //! \param lastHeartbeats Map containing when the last heartbeat arrived from each client.
    //!
    void deleteClient(const Message& m, HBClients& clients, HBMap& lastHeartbeats);

    //! Stop the heartbeat thread. This is called when the thread can't send to
    //! the local socket.
    //!
    void failedSendServer(const Message& m);

    //! Update book-keeping on client heartbeats. Called when a heartbeat message arrives
    //! from a client.
    //!
    //! \param m The HEARTBEAT message from the client.
    //! \param lastHeartbeats Map containing when the last heartbeat arrived from each client.
    //! 
    void receiveHeartbeat(const Message& m, HBMap& lastHeartbeats);

    //! Update book-keeping on client heartbeats. Called every time the socket is polled.
    //!
    //! \param clients List of clients being monitored.
    //! \param lastHeartbeats Map containing when the last heartbeat arrived from each client.
    //! \param router A reference to the socket router.
    //! \param msTimeout The heartbeat timeout in milliseconds.
    //!
    void monitorTimeouts(HBClients& clients, HBMap& lastHeartbeats, SocketRouter& router, uint msTimeout);

    //! Send heartbeats to all the clients if necessary.
    //!
    //! \param clients List of clients being monitored.
    //! \param lastHbTime The last time this thread sent out heartbeats.
    //! \param router A reference to the socket router.
    //! \param msFrequency The number of milliseconds between each heartbeat.
    //!
    void sendHeartbeats(HBClients& clients, hrc::time_point& lastHbTime, SocketRouter& router, uint msFrequency);

    ServerHeartbeat::ServerHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings)
        : msFrequency_(settings.msRate), msPollingFrequency_(settings.msPollRate), msTimeout_(settings.msTimeout),
        router_("HB"), running_(true)
    {
      // Create the router's socket
      std::unique_ptr<zmq::socket_t> heartbeat(new zmq::socket_t(context, ZMQ_PAIR));
      heartbeat->connect(SERVER_HB_SOCKET_ADDR.c_str());
      router_.add_socket(SocketID::HEARTBEAT, heartbeat);

      // Specify functionality 
      auto onRcvHELLO = [&] (const Message&m)
      { insertClient(m, clients_, lastHeartbeats_);};
      auto onRcvGOODBYE = [&] (const Message&m)
      { deleteClient(m, clients_, lastHeartbeats_);};
      auto onRcvHEARTBEAT = [&] (const Message&m)
      { receiveHeartbeat(m, lastHeartbeats_);};

      auto timeouts = [&] ()
      { monitorTimeouts(clients_, lastHeartbeats_, router_, msTimeout_);};
      auto send = [&] ()
      { sendHeartbeats(clients_, lastSendTime_, router_, msFrequency_);};

      // Bind to router
      router_(SocketID::HEARTBEAT).onRcvHELLO.connect(onRcvHELLO);
      router_(SocketID::HEARTBEAT).onRcvGOODBYE.connect(onRcvGOODBYE);
      router_(SocketID::HEARTBEAT).onRcvHEARTBEAT.connect(onRcvHEARTBEAT);
      router_(SocketID::HEARTBEAT).onPoll.connect(timeouts);
      router_(SocketID::HEARTBEAT).onPoll.connect(send);
      router_(SocketID::HEARTBEAT).onFailedSend.connect(failedSendServer);
      
      LOG(INFO)<< "Server heartbeat initialising";
      lastSendTime_ = std::chrono::high_resolution_clock::now();
      router_.start(msPollingFrequency_, running_); // milliseconds between polling loops
    }

    ServerHeartbeat::~ServerHeartbeat()
    {
      running_ = false; // should finish the router thread
    }
    
    void insertClient(const Message& m, HBClients& clients, HBMap& lastHeartbeats)
    {
      clients.insert(m.address.back());
      lastHeartbeats.insert(std::pair<std::string, hrc::time_point>(m.address.back(), hrc::now()));
    }

    void deleteClient(const Message& m, HBClients& clients, HBMap& lastHeartbeats)
    {
      VLOG(1) << " HB system received GOODBYE from " << m.address.back();
      clients.erase(clients.find(m.address.back()));
      lastHeartbeats.erase(m.address.back());
    }

    void failedSendServer(const Message& m)
    {
      LOG(ERROR)<< "HEARTBEAT FAILED TO SEND TO LOCAL SOCKET";
      throw std::runtime_error("heartbeat thread send failed");
    }

    void receiveHeartbeat(const Message& m, HBMap& lastHeartbeats)
    {
      auto deltaT = hrc::now() - lastHeartbeats[m.address.back()];
      uint deltams = std::chrono::duration_cast < std::chrono::milliseconds > (deltaT).count();
      VLOG(4) << "Heartbeat from " << m.address.back() << " with delta T = " << deltams << "ms";
      lastHeartbeats[m.address.back()] = hrc::now();
    }

    void monitorTimeouts(HBClients& clients, HBMap& lastHeartbeats, SocketRouter& router, uint msTimeout)
    {
      for (auto addr : clients)
      {
        auto msElapsed = std::chrono::duration_cast < std::chrono::milliseconds > (hrc::now() - lastHeartbeats[addr]).count();
        if (msElapsed > msTimeout)
        {
          stateline::comms::Message gb( { addr }, stateline::comms::GOODBYE);
          VLOG(1) << "Heartbeat system sending GOODBYE on behalf of " << addr;
          router.send(SocketID::HEARTBEAT, Message( { addr }, stateline::comms::GOODBYE));
          clients.erase(clients.find(addr));
          lastHeartbeats.erase(addr);
        }
      }
    }

    void sendHeartbeats(HBClients& clients, hrc::time_point& lastHbTime, SocketRouter& router, uint msFrequency)
    {
      auto currentTime = hrc::now();
      auto timeSinceLastHb = std::chrono::duration_cast < std::chrono::milliseconds > (currentTime - lastHbTime);
      if (timeSinceLastHb >= std::chrono::milliseconds(msFrequency))
      {
        for (auto addr : clients)
        {
          VLOG(4) << "Sending HEARTBEAT to " << addr;
          router.send(SocketID::HEARTBEAT, Message( { addr }, HEARTBEAT));
          lastHbTime = hrc::now();
        }
      }
    }

  } // namespace comms
} // namespace stateline
