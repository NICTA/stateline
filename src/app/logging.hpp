//!
//! Helper functions for setting up logging.
//!
//! \file app/logging.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

namespace stateline
{
  //! Initialise the logging system
  //
  //! \param logLevel The level of logging. Can be TEST, INFO, DEBUG, TRACE.
  //!        increasingly quiet.
  //! \param stdErr Switch to enable logging to stdError.
  //! \param directory The directory to log to if stdErr logging is disabled.
  //!
  void initLogging(const std::string& logLevel, bool stdOut=true, const std::string& filename=".");
} // namespace stateline
