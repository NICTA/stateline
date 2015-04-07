//!
//! Implementation of heartbeat routers that deal with client heartbeating
//! sending and receiving.
//!
//! \file comms/clientheartbeat.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/clientheartbeat.hpp"
#include <stdexcept>
#include <glog/logging.h>
#include "comms/settings.hpp"

namespace stateline
{
  namespace comms
  {
    //! Stop the heartbeat thread. This is called when the thread can't send to
    //! the local socket.
    //!
    void failedSend(const Message& m);

    //! Update book-keeping on server heartbeats. Called every time the socket is polled.
    //!
    //! \param lastReceivedTime The time that the client last received a heartbeat from the server.
    //! \param router A reference to the socket router.
    //! \param msTimeout The heartbeat timeout in milliseconds.
    //!
    void monitorTimeout(hrc::time_point& lastReceivedTime, SocketRouter& router, uint msTimeout, bool& running);

    //! Update book-keeping on server heartbeats. Called when the socket receives a heartbeat from the server.
    //!
    //! \param lastReceivedTime The time that the client last received a heartbeat from the server.
    //!
    void heartbeatArrived(hrc::time_point& lastReceivedTime);

    //! Send a heartbeat to the server if necessary.
    //!
    //! \param lastSendTime The last time this client sent a heartbeat.
    //! \param router A reference to the socket router.
    //! \param msFrequency The number of milliseconds between each heartbeat.
    //!
    void sendHeartbeat(hrc::time_point& lastSendTime, SocketRouter& router, uint msFrequency);

    ClientHeartbeat::ClientHeartbeat(zmq::context_t& context, const HeartbeatSettings& settings)
        : msFrequency_(settings.msRate), msPollingFrequency_(settings.msPollRate), msTimeout_(settings.msTimeout),
        router_("HB"), running_(true), socket_(context, ZMQ_PAIR)
    {
      socket_.connect(CLIENT_HB_SOCKET_ADDR.c_str());
      router_.add_socket(SocketID::HEARTBEAT, socket);

      // Specify functionality 
      auto onRcvHEARTBEAT = [&] (const Message &)
      { heartbeatArrived(lastReceivedTime_);};
      auto onRcvGOODBYE = [&] (const Message &)
      { running_ = false;};

      auto onPoll = [&] ()
      { monitorTimeout(lastReceivedTime_, router_, msTimeout_, running_);
        sendHeartbeat(lastSendTime_, router_, msFrequency_); 
      };

      // Bind to router
      router_.bind(onRcvHEARTBEAT, HEARTBEAT, SocketID::HEARTBEAT);
      router_(onRcvGOODBYE, GOODBYE, SocketID::HEARTBEAT);
      router_.bindOnPoll(onPoll, SocketID::HEARTBEAT);
      
      // Start router 
      lastSendTime_ = std::chrono::high_resolution_clock::now();
      lastReceivedTime_ = std::chrono::high_resolution_clock::now();
      router_.poll(msPollingFrequency_, running_); // milliseconds between polling loops
    }

    ClientHeartbeat::~ClientHeartbeat()
    {
      running_ = false;
    }

    void failedSend(const Message &)
    {
      LOG(ERROR)<< "HEARTBEAT FAILED TO SEND TO LOCAL SOCKET";
      throw std::runtime_error("heartbeat thread send failed");
    }

    void monitorTimeout(hrc::time_point& lastReceivedTime, SocketRouter& router, uint msTimeout, bool& running)
    {
      auto msElapsed = std::chrono::duration_cast < std::chrono::milliseconds > (hrc::now() - lastReceivedTime).count();
      if (msElapsed > msTimeout)
      {
        VLOG(1) << "Heartbeat system sending GOODBYE on behalf of server";
        router.send(SocketID::HEARTBEAT, Message(stateline::comms::GOODBYE));
        running = false;
      }
    }

    void heartbeatArrived(hrc::time_point& lastReceivedTime)
    {
      auto deltaT = hrc::now() - lastReceivedTime;
      uint deltams = std::chrono::duration_cast < std::chrono::milliseconds > (deltaT).count();
      VLOG(4) << "Heartbeat with delta T = " << deltams << "ms";
      lastReceivedTime = hrc::now();
    }

    void sendHeartbeat(hrc::time_point& lastSendTime, SocketRouter& router, uint msFrequency)
    {
      auto currentTime = hrc::now();
      auto timeSinceLastHb = std::chrono::duration_cast < std::chrono::milliseconds > (currentTime - lastSendTime);
      if (timeSinceLastHb >= std::chrono::milliseconds(msFrequency))
      {
        VLOG(4) << "Sending heartbeat...";
        router.send(SocketID::HEARTBEAT, Message(stateline::comms::HEARTBEAT));
        lastSendTime = hrc::now();
      }
    }

  } // namespace comms
} // namespace stateline
