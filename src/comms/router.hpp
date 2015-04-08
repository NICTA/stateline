//!
//! A router that implements polling on multiple zeromq sockets,
//! and a signal-based callback system depending on the socket and the subject
//! of the message. Implementing STATELINEP-SW (STATELINE Server-Wrapper Protocol)
//!
//! \file comms/router.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <memory>
#include <future>
// Project
#include "comms/messages.hpp"
#include "comms/socket.hpp"

namespace stateline
{
  namespace comms
  {

    //! The callback function upon receipt of a message
    typedef std::function<void(const Message& m)> Callback;

    //! Implements polling and configurable routing between an
    //! arbitrary number of (pre-constructed) sockets. Functionality
    //! is attached through a signal interface.
    //!
    class SocketRouter
    {
      public:
        //! Create a new socket router.
        //!
        SocketRouter(const std::string& name, const std::vector<Socket*>& sockets);

        //! Clean up resources used by the socket router.
        //!
        ~SocketRouter();

        void bind(uint socketIndex, const Subject& s, const Callback& f);

        void bindOnPoll(const std::function<void(void)>& f);

        //! Start the router polling with a polling loop frequency
        void poll(int msPerPoll, bool& running);

      private:
        std::string name_;
        std::vector<Socket*> sockets_; // TODO: do we need to store the sockets?
        std::vector<zmq::pollitem_t> pollList_;
        std::vector<Callback> callbacks_;
        std::function<void(void)> onPoll_;
    };

  } // namespace stateline
}// namespace comms
