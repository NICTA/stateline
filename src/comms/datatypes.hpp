//!
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
      //! of job
      uint type;
      //! common to all jobs
      std::string globalData;
      //! specific to this job
      std::string jobData;
    };

    //! Abstraction of job results.
    struct ResultData
    {
      //! of job
      uint type;
      //! results data
      std::string data;
    };

    //! Convert list of uints in string form to vector of uint.
    inline std::vector<uint> stringToIDs(const std::string& s)
    {
      std::istringstream lineStream(s);
      std::vector<uint> ids;
      uint num;
      while (lineStream >> num)
        ids.push_back(num);
      return ids;
    }

    //! Convert vector of uints to list of uints in string form seperated by space.
    inline std::string idsToString(const std::vector<uint>& ids)
    {
      std::stringstream result;
      std::copy(ids.begin(), ids.end(), std::ostream_iterator<uint>(result, " "));
      std::string s = result.str();
      s.erase(s.end() - 1);
      return s;
    }

  } // namespace comms
} // namespace obsidian
