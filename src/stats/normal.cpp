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

    NormalDetail::NormalDetail(const Eigen::MatrixXd &cov)
    {
      assert(cov.rows() == cov.cols());

      // TODO: check whether the decomposition was successful
      covL = cov.llt().matrixL();
      covLInv = covL.lu().inverse();
      logDet = covLInv.diagonal().array().log().sum();
    }

    Normal::Normal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov)
      : NormalDetail(cov),
        Multivariate(mean.size(), -0.5 * mean.size() * log2PI + this->logDet),
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
    Eigen::VectorXd var(const Normal &d)
    {
      return cov(d).diagonal();
    }

    template <>
    double ulogpdf(const Normal &d, const Eigen::VectorXd &x)
    {
      return -0.5 * (d.covLInv * (x - mean(d))).squaredNorm();
    }
  }
}
