//!
//! Signal handler.
//!
//! \file signal.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <atomic>
#include <csignal>
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include <future>

namespace stateline
{
  namespace global
  {
    std::atomic<bool> interruptedBySignal;
  }

  namespace init
  {
    void handleSignal(int sig)
    {
      VLOG(1) << "Caught Signal of type " << sig;
      stateline::global::interruptedBySignal = true;
    }

    void threadHandle()
    {
      std::signal(SIGINT, handleSignal);
      std::signal(SIGTERM, handleSignal);
      while(!global::interruptedBySignal)
      {
        std::this_thread::sleep_for(std::chrono::seconds(60));
      }
    }

    void initialiseSignalHandler()
    {
      // Install a signal handler
      // std::signal(SIGINT, handleSignal);
      // std::signal(SIGTERM, handleSignal);
      std::signal(SIGINT, SIG_IGN);
      std::signal(SIGTERM, SIG_IGN);
      // Let google handle segfaults
      google::InstallFailureSignalHandler();
      std::thread(threadHandle).detach();
    }

  }
}
