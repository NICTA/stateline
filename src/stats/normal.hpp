//!
//! Contains the interface for the normal distribution.
//!
//! \file stats/normal.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "stats/multivariate.hpp"

namespace stateline
{
  namespace stats
  {
    class Normal : Multivariate
    {
      public:
        Normal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov);

        Eigen::VectorXd mean() const;
        Eigen::MatrixXd cov() const;

      private:
        Eigen::VectorXd mean_;
        Eigen::MatrixXd cov_;

        // LLT decomposition for drawing samples
        Eigen::MatrixXd covL_;

        // LU decomposition for inverting the covariance matrix
        Eigen::MatrixXd covLInv_;

        // Cached values used in PDF calculation 
        double logDet_;
        double norm_;

        // TODO: urgh
        friend double logpdf<>(const Normal &d, const Eigen::VectorXd &x);
    };

    template <>
    Eigen::VectorXd var(const Normal &d);

    template <>
    double logpdf(const Normal &d, const Eigen::VectorXd &x);
  }
}
