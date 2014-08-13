//!
//! Settings for database.
//!
//! \file db/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

namespace stateline
{
  //! Settings for interacting with the MCMC database.
  //!
  struct DBSettings
  {
    //! Path to the folder directory containing the database.
    std::string directory;

    //! The size of the database cache in megabytes.
    double cacheSizeMB;

    //! Default settings
    static DBSettings Default()
    {
      DBSettings settings = {};
      settings.directory = "chainDB";
      settings.cacheSizeMB = 100;
      return settings;
    }
  };
}
