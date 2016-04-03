//!
//! Signal handler.
//!
//! \file signal.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <atomic>

namespace stateline
{
  namespace global
  {
    extern std::atomic_bool interruptedBySignal;
  }

  namespace init
  {
    void handleSignal(int sig);
    void threadHandle();
    void initialiseSignalHandler();
  }
}
