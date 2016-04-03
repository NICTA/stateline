//!
//! Implementation of signal handler.
//!
//! \file signal.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <csignal>
#include <easylogging/easylogging++.h>
#include <chrono>
#include <thread>
#include <future>

#include "app/signal.hpp"

std::atomic_bool stateline::global::interruptedBySignal = ATOMIC_VAR_INIT(false);

void stateline::init::handleSignal(int sig)
{
  VLOG(1) << "Caught Signal of type " << sig;
  stateline::global::interruptedBySignal = true;
}

void stateline::init::threadHandle()
{
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  while(!global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }
}

void stateline::init::initialiseSignalHandler()
{
  // Install a signal handler
  // std::signal(SIGINT, handleSignal);
  // std::signal(SIGTERM, handleSignal);
  std::signal(SIGINT, SIG_IGN);
  std::signal(SIGTERM, SIG_IGN);
  std::thread(threadHandle).detach();
}
