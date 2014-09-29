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
      : Multivariate(mean.size()),
        mean_(mean), diag_(cov)
    {
      assert(mean.size() == cov.size());
    }

    Eigen::VectorXd DiagNormal::mean() const
    {
      return mean_;
    }

    Eigen::VectorXd DiagNormal::diag() const
    {
      return diag_;
    }

    template <>
    Eigen::VectorXd mean(const DiagNormal &d)
    {
      return d.mean();
    }

    template <>
    Eigen::VectorXd var(const DiagNormal &d)
    {
      return d.diag();
    }

    template <>
    double logpdf(const DiagNormal &d, const Eigen::VectorXd &x)
    {
      return -0.5 * ((x - mean(d)) * (x - mean(d))).cwiseQuotient(d.diag()).sum();
    }
  }
}
