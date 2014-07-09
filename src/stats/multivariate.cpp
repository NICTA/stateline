//!
//! Contains implementation of the base class of all multivariate distributions.
//!
//! \file stats/multivariate.cpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "multivariate.hpp"

namespace stateline
{
  namespace stats
  {
    Multivariate::Multivariate(std::size_t length, double lognorm)
      : length_(length), norm_(std::exp(lognorm)), lognorm_(lognorm)
    {
    }

    std::size_t Multivariate::length() const
    {
      return length_;
    }

    double Multivariate::norm() const
    {
      return norm_;
    }

    double Multivariate::lognorm() const
    {
      return lognorm_;
    }
  }
}
