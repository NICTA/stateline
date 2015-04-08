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
    void monitorTimeouts(HBClients& clients, HBMap& lastHeartbeats, Socket& socket, uint msTimeout);

    //! Send heartbeats to all the clients if necessary.
    //!
    //! \param clients List of clients being monitored.
    //! \param lastHbTime The last time this thread sent out heartbeats.
    //! \param router A reference to the socket router.
    //! \param msFrequency The number of milliseconds between each heartbeat.
    //!
    void sendHeartbeats(HBClients& clients, hrc::time_point& lastHbTime, Socket& socket, uint msFrequency);

    ServerHeartbeat::ServerHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings)
        : socket_(context, ZMQ_PAIR, "toServer"),
          router_("HB", { &socket_ }),
          msPollRate_(settings.msPollRate),
          running_(false)
    {
      socket_.connect(SERVER_HB_SOCKET_ADDR);

      // Specify functionality
      auto rcvHello = [&](const Message& m) { insertClient(m, clients_, lastHeartbeats_); };
      auto rcvGoodbye = [&](const Message& m) { deleteClient(m, clients_, lastHeartbeats_); };
      auto rcvHeartbeat = [&](const Message& m) { receiveHeartbeat(m, lastHeartbeats_); };

      auto onPoll = [&]()
      {
        monitorTimeouts(clients_, lastHeartbeats_, socket_, settings.msTimeout);
        sendHeartbeats(clients_, lastSendTime_, socket_, settings.msRate);
      };

      // Bind to router
      const uint CLIENT_SOCKET = 0;

      router_.bind(CLIENT_SOCKET, HELLO, rcvHello);
      router_.bind(CLIENT_SOCKET, GOODBYE, rcvGoodbye);
      router_.bind(CLIENT_SOCKET, HEARTBEAT, rcvHeartbeat);
      router_.bindOnPoll(onPoll);
    }

    void ServerHeartbeat::start()
    {
      running_ = true;
      lastSendTime_ = std::chrono::high_resolution_clock::now();
      router_.poll(msPollRate_, running_);
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

    void receiveHeartbeat(const Message& m, HBMap& lastHeartbeats)
    {
      auto deltaT = hrc::now() - lastHeartbeats[m.address.back()];
      uint deltams = std::chrono::duration_cast < std::chrono::milliseconds > (deltaT).count();
      VLOG(4) << "Heartbeat from " << m.address.back() << " with delta T = " << deltams << "ms";
      lastHeartbeats[m.address.back()] = hrc::now();
    }

    void monitorTimeouts(HBClients& clients, HBMap& lastHeartbeats, Socket& socket, uint msTimeout)
    {
      for (const auto& addr : clients)
      {
        auto msElapsed = std::chrono::duration_cast < std::chrono::milliseconds > (hrc::now() - lastHeartbeats[addr]).count();
        if (msElapsed > msTimeout)
        {
          VLOG(1) << "Heartbeat system sending GOODBYE on behalf of " << addr;
          socket.send({{ addr }, GOODBYE});
          clients.erase(clients.find(addr));
          lastHeartbeats.erase(addr);
        }
      }
    }

    void sendHeartbeats(HBClients& clients, hrc::time_point& lastHbTime, Socket& socket, uint msFrequency)
    {
      auto currentTime = hrc::now();
      auto timeSinceLastHb = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastHbTime);
      if (timeSinceLastHb >= std::chrono::milliseconds(msFrequency))
      {
        for (const auto& addr : clients)
        {
          VLOG(4) << "Sending HEARTBEAT to " << addr;
          socket.send({{ addr }, HEARTBEAT});
          lastHbTime = hrc::now();
        }
      }
    }

  } // namespace comms
} // namespace stateline
