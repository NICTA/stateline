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

#include <csignal>

namespace obsidian
{
  namespace global
  {
    volatile bool interruptedBySignal;
  }

  namespace init
  {
    void handleSignal(int sig)
    {
      obsidian::global::interruptedBySignal = true;
    }

    void initialiseSignalHandler()
    {
      // Install a signal handler
      std::signal(SIGINT, handleSignal);
      std::signal(SIGTERM, handleSignal);
    }
  }
}
