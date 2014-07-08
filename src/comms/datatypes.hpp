//!
//! Contains comms data structures representing jobs and results.
//!
//! \file comms/datatypes.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <iterator>
#include <string>
#include <sstream>
#include <cctype>

namespace stateline
{
  namespace comms
  {
    //! Abstraction of job specification.
    struct JobData
    {
      //! Type of job
      uint type;

      //! Data common to all jobs
      std::string globalData;

      //! Data specific to this job
      std::string jobData;
    };

    //! Abstraction of job results.
    struct ResultData
    {
      //! Type of job
      uint type;

      //! Results data
      std::string data;
    };

  } // namespace comms
} // namespace stateline
