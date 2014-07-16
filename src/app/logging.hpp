//!
//! Helper functions for setting up logging.
//!
//! \file app/console.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

namespace stateline
{
  //! Initialise the logging system
  //
  //! \param appName The name of the executable
  //! \param logLevel The numerical level of logging. <0 gets increasingly
  //!        verbose for debugging. 0 is release logging, and >0 is 
  //!        increasingly quiet.
  //! \param stdErr Switch to enable logging to stdError.
  //! \param directory The directory to log to if stdErr logging is disabled.
  //!
  void initLogging(const std::string &appName, int logLevel, bool stdErr,
      const std::string &directory);
} // namespace stateline
