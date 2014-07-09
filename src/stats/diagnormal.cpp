//!
//! Contains the implementation of normal distributions with diagonal covariance
//! matrix.
//!
//! \file stats/diagnormal.cpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/diagnormal.hpp"

namespace stateline
{
  namespace stats
  {
    DiagNormal::DiagNormal(const Eigen::VectorXd &mean, const Eigen::VectorXd &cov)
      : Multivariate(mean.size()), mean_(mean), diag_(cov)
    {
      assert(mean.size() == cov.size());

      norm_ = -std::log(std::pow(2.0 * M_PI, mean.size() * 0.5) * diag_.prod());
    }

    template <>
    Eigen::VectorXd var(const DiagNormal &d)
    {
      return d.diag();
    }

    template <>
    double logpdf(const DiagNormal &d, const Eigen::VectorXd &x)
    {
      return d.norm_ - 0.5 * (x - mean(d)).cwiseQuotient(d.diag()).squaredNorm();
    }
  }
}
