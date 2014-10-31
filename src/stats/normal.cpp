//!
//! Contains the implementation of the normal distribution.
//!
//! \file stats/normal.cpp
//! \author Darren Shen
//! \author Alistair Reid
//! \date 2014
//! \license Lesser General Public License version 3 or later
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

    Normal::Normal(std::size_t ndims)
      : NormalDetail(Eigen::MatrixXd::Identity(ndims, ndims)),
        Multivariate(ndims),
        mean_(Eigen::MatrixXd::Zero(ndims, ndims)),
        cov_(Eigen::MatrixXd::Identity(ndims, ndims))
    {
    }

    Normal::Normal(const Eigen::VectorXd &mean)
      : NormalDetail(Eigen::MatrixXd::Identity(mean.size(), mean.size())),
        Multivariate(mean.size()),
        mean_(mean),
        cov_(Eigen::MatrixXd::Identity(mean.size(), mean.size()))
    {
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
