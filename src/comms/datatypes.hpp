//!
//! Contains comms data structures representing jobs and results.
//!
//! \file comms/datatypes.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <chrono>

namespace stateline
{
  namespace comms
  {
    //! High resolution clock used for heartbeating
    typedef std::chrono::high_resolution_clock hrc;

  } // namespace comms
} // namespace stateline
