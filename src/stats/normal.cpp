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
    NormalDetail::NormalDetail(const Eigen::MatrixXd &cov)
    {
      assert(cov.rows() == cov.cols());

      // TODO: check whether the decomposition was successful
      covL = cov.llt().matrixL();
      covLInv = covL.lu().inverse();
    }

    Normal::Normal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov)
      : NormalDetail(cov),
        Multivariate(mean.size()),
        mean_(mean), cov_(cov)
    {
      assert(mean.size() == cov.rows());
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
    Eigen::VectorXd mean(const Normal &d)
    {
      return d.mean();
    }

    template <>
    Eigen::MatrixXd cov(const Normal &d)
    {
      return d.cov();
    }

    template <>
    double logpdf(const Normal &d, const Eigen::VectorXd &x)
    {
      return -0.5 * (d.covLInv * (x - mean(d))).squaredNorm();
    }
  }
}
