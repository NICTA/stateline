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

namespace stateline
{
  namespace global
  {
    volatile bool interruptedBySignal;
  }

  namespace init
  {
    void handleSignal(int sig)
    {
      stateline::global::interruptedBySignal = true;
    }

    void initialiseSignalHandler()
    {
      // Install a signal handler
      std::signal(SIGINT, handleSignal);
      std::signal(SIGTERM, handleSignal);
    }
  }
}
