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
    constexpr double log2PI = std::log(2.0 * M_PI);

    DiagNormal::DiagNormal(const Eigen::VectorXd &mean, const Eigen::VectorXd &cov)
      : Multivariate(mean.size(), -0.5 * mean.size() * log2PI * diag_.prod()),
        mean_(mean), diag_(cov)
    {
      assert(mean.size() == cov.size());
    }

    template <>
    Eigen::VectorXd var(const DiagNormal &d)
    {
      return d.diag();
    }

    template <>
    double logpdf(const DiagNormal &d, const Eigen::VectorXd &x)
    {
      return d.norm() - 0.5 * (x - mean(d)).cwiseQuotient(d.diag()).squaredNorm();
    }
  }
}
