//!
//! Contains the implementation of the truncated normal distribution.
//!
//! \file stats/truncnormal.cpp
//! \author Darren Shen
//! \author Alistair Reid
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/truncnormal.hpp"

namespace stateline
{
  namespace stats
  {
    TruncNormal::TruncNormal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov,
        const Eigen::VectorXd &min, const Eigen::VectorXd &max)
      : Normal(mean, cov),
        min_(min), max_(max)
    {
    }

    template <>
    bool insupport(const TruncNormal &d, const Eigen::VectorXd &x)
    {
      // Check that all dimensions are within the bounds of the distribution.
      return ((d.min().array() < x.array()) && (x.array() < d.max().array())).all();
    }

    template <>
    double ulogpdf(const TruncNormal &d, const Eigen::VectorXd &x)
    {
      if (insupport(d, x))
      {
        return ulogpdf(*static_cast<const Normal *>(&d), x);
      }
      else
      {
        return -std::numeric_limits<double>::infinity();
      }
    }
  }
}
