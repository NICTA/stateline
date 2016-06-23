//!
//! Base class for handling socket events.
//!
//! \file comms/handler.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "messages.hpp"
#include "socket.hpp"

namespace stateline
{
namespace comms
{

template <class T>
class SocketHandler
{
public:
  void onHello() { }
  void onRequest() { }
  void onResult() { }
  void onHeartbeat() { }
  void onGoodbye() { }
  void onDisconnect() { }

};

}
