//!
//! Implementation of signal handler.
//!
//! \file signal.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

// DO NOT USE
// C++ std::signal causes undefined behaviour in multithreaded programs
// http://en.cppreference.com/w/cpp/utility/program/signal

/*
#include <csignal>
#include <easylogging/easylogging++.h>
#include <chrono>
#include <thread>
#include <future>

#include "app/signal.hpp"

std::atomic_bool stateline::global::interruptedBySignal = ATOMIC_VAR_INIT(false);

void handleSignal(int sig)
{
  SL_LOG(INFO) << "Caught Signal of type " << sig;
  stateline::global::interruptedBySignal = true;
}

void initialiseSignalHandler()
{
  // Install a signal handler
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  std::signal(SIGPROF, handleSignal);
  //std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  //std::thread(threadHandle).detach();
}

bool isRunning()
{
  return !interruptedBySignal.get();
}

}*/
