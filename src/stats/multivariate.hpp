//!
//! Contains the base class of all multivariate distributions.
//!
//! \file stats/stats.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "stats.hpp"

namespace stateline
{
  namespace stats
  {
    class Multivariate
    {
      public:
        Multivariate(std::size_t length, double lognorm = 0.0);

        std::size_t length() const;
        double norm() const;
        double lognorm() const;

      private:
        std::size_t length_;
        double norm_;
        double lognorm_;
    };
  }
}
