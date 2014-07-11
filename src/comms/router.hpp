//!
//! A router that implements polling on multiple zeromq sockets,
//! and a signal-based callback system depending on the socket and the subject
//! of the message. Implementing STATELINEP-SW (STATELINE Server-Wrapper Protocol)
//!
//! \file comms/router.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <memory>
#include <future>
//Prerequisites
#include <boost/signals2/signal.hpp>
#include <boost/bimap.hpp>
// Project
#include "comms/messages.hpp"
#include "comms/transport.hpp"

//! Type representing a signal that takes a message as an argument.
typedef boost::signals2::signal<void(const stateline::comms::Message&)> MsgSignal;

//! Type represent a signal that takes no arguments.
typedef boost::signals2::signal<void()> VoidSignal;

namespace stateline
{
  namespace comms
  {
    //! Enums to give each type of socket an index.
    //!
    enum class SocketID
    {
      REQUESTER, MINION, WORKER, NETWORK, HEARTBEAT, ALPHA, BETA
    };

    //! Print a socket ID for logging and debugging purposes.
    //!
    //! \param os The output stream.
    //! \param id The socket ID to print.
    //!
    std::ostream& operator<<(std::ostream& os, const SocketID& id);

    //! A type that maps between indices in an array, and SocketID objects.
    typedef boost::bimap<SocketID, uint> IndexBiMap;

    //! Represents the callback signals for a socket.
    //!
    struct SocketHandler
    {
      //! The socket receives a hello message.
      MsgSignal onRcvHELLO;

      //! The socket receives a heartbeat message.
      MsgSignal onRcvHEARTBEAT;

      //! The socket receives a problem spec message.
      MsgSignal onRcvPROBLEMSPEC;

      //! The socket receives a job request message.
      MsgSignal onRcvJOBREQUEST;

      //! The socket receives a job.
      MsgSignal onRcvJOB;

      //! The socket receives a job swap message.
      MsgSignal onRcvJOBSWAP;

      //! The socket receives an all done message.
      MsgSignal onRcvALLDONE;

      //! The socket receives a goodbye message.
      MsgSignal onRcvGOODBYE;

      //! The socket receives a failed send message.
      MsgSignal onFailedSend;

      //! The socket is polled.
      VoidSignal onPoll;
    };

    //! Implements polling and configurable routing between an
    //! arbitrary number of (pre-constructed) zeromq sockets. Functionality
    //! is attached through a signal interface.
    //!
    class SocketRouter
    {
      public:
        //! Create a new socket router.
        //!
        SocketRouter();

        //! Clean up resources used by the socket router.
        //!
        ~SocketRouter();

        //! Add a socket to be controlled by the router
        //!
        //! \param idx The SocketID index that will refer to the socket
        //! \param socket The unique_ptr holding the socket (we'll take it)
        //!
        void add_socket(SocketID idx, std::unique_ptr<zmq::socket_t>& socket);

        //! Start the router polling with blocking
        //!
        //! It is critical that the send and receive calls are only used from
        //! functions connected to the signals once polling is started. This is
        //! because the sockets themselves are not thread safe.
        //!
        void start();

        //! Start the router polling with a polling loop frequency
        //!
        //! It is critical that the send and receive calls are only used from
        //! functions connected to the signals once polling is started. This is
        //! because the sockets themselves are not thread safe.
        //!
        void start(int msPerPoll);

        //! Access the handler (ie the signal interface) for a particular socket.
        //!
        //! \param id The socket ID of the socket.
        //! \return A reference to the socket callback struct.
        //!
        SocketHandler& operator()(const SocketID& id);

        //! Stop the router from polling.
        //!
        void stop();

        //! Send a message through a socket.
        //!
        //! \warning Do not use this once polling has started except from attached
        //!          function callbacks (which run in the same thread as the polling
        //!          loop).
        //!
        //! \param id The socket ID.
        //! \param msg The message to send.
        //!
        void send(const SocketID& id, const Message& msg);

        //! Receive a message from a socket
        //!
        //! \warning Do not use this once polling has started except from attached
        //!          function callbacks (which run in the same thread as the polling
        //!          loop).
        //!
        //! \param id The socket ID.
        //! \return The message received.
        //!
        Message receive(const SocketID& id);

      private:
        //! Poll the sockets for received messages with timeout
        //! This runs in its own thread.
        //!
        //! \param microsecondsWait The wait on each polling loop,
        //!                         with -1 indicating a blocking poll
        //!
        bool poll(int microsecondsWait);

        //! Called when the socket receives a message.
        //!
        void receive(zmq::socket_t& socket, SocketHandler& h, const SocketID& idx);

        // Member variables
        std::atomic_bool running_;
        std::future<bool> threadReturned_;
        std::vector<std::unique_ptr<SocketHandler>> handlers_;
        std::vector<std::unique_ptr<zmq::socket_t>> sockets_;
        std::vector<zmq::pollitem_t> pollList_;
        IndexBiMap indexMap_;
    };

  } // namespace obsidian
}// namespace comms
