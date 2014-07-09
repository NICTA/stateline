//!
//! Contains the implementation of the normal distribution.
//!
//! \file stats/normal.cpp
//! \author Darren Shen
//! \author Alistair Reid
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/normal.hpp"

namespace stateline
{
  namespace stats
  {
    constexpr double log2PI = 1.837877066409345;

    Normal::Normal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov)
      : Multivariate(mean.size()), mean_(mean), cov_(cov)
    {
      assert(mean.size() == cov.rows());
      assert(cov.rows() == cov.cols());

      covL_ = cov.llt().matrixL();
      // TODO: check whether the decomposition was successful

      Eigen::FullPivLU<Eigen::MatrixXd> lu(covL_);
      covLInv_ = lu.inverse();
  
      logDet_ = covLInv_.diagonal().array().log().sum();
      norm_ = -0.5 * mean.size() * log2PI;
    }

    Eigen::VectorXd Normal::mean() const
    {
      return mean_;
    }

    Eigen::MatrixXd Normal::cov() const
    {
      return cov_;
    }

    template <>
    Eigen::VectorXd var(const Normal &d)
    {
      return cov(d).diagonal();
    }

    template <>
    double logpdf(const Normal &d, const Eigen::VectorXd &x)
    {
      return d.norm_ + d.logDet_ - 0.5 * (d.covLInv_ * (x - mean(d))).squaredNorm();
    }
  }
}
