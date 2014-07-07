//!
//! Contains the functions to communicate GDF datatypes across zmq in accordance
//! with the GDFP-SW (GDF Server-Worker protocol)
//!
//! \file comms/transport.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <string>
//Prerequisites
#include <zmq.hpp>
// Project
#include "comms/messages.hpp"

namespace stateline
{
  namespace comms
  {
    //! Read a string form a ZMQ socket. This is a blocking call.
    //!
    //! \param socket The socket to read from.
    //! \return A string containing the data read from the socket.
    //!
    std::string receiveString(zmq::socket_t & socket);
 
    //! Send a string over a ZMQ socket.
    //!
    //! \param socket The socket to send the string over.
    //! \param string The string data to send.
    //! \return Whether the send was successful.
    //!
    bool sendString(zmq::socket_t & socket, const std::string & string);

    //! Send a string over a ZMQ socket as a chunk of a multipart message.
    //!
    //! \param socket The socket to send the string over.
    //! \param string The string data to send.
    //! \return Whether the send was successful.
    //!
    bool sendStringPart(zmq::socket_t & socket, const std::string & string);

    //! Receive a (possibly multi-part) message conforming to GDFP-SW comms
    //! protocal and provide it as a stateline::comms::Message class.
    //!
    //! \param socket The socket the message is coming into.
    //! \return The Message object received.
    //!
    Message receive(zmq::socket_t& socket);

    //! Send a (possibly multi-part) message conforming to GDFP-SW comms
    //! protocol.
    //!
    //! \param socket The socket the message is coming into.
    //! \param message The stateline::comms::Message to send.
    //!
    void send(zmq::socket_t& socket, const Message& message);

    //! Computes a random socket ID conforming to zeromq requirements for the
    //! string (not starting with a zero etc, see zeromq doco).
    //!
    //! \return A socket id string.
    //!
    std::string randomSocketID();

    //! Sets the ID of a socket. The socket must be a DEALER or A REQ.
    //!
    //! \param id The socket ID.
    //! \param socket The socket that is being assigned the ID.
    //!
    void setSocketID(const std::string& id, zmq::socket_t & socket);

  } // namespace comms
} // namespace obsidian
