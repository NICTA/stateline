//!
//! Contains the interface for normal distributions with diagonal covariance
//! matrix.
//!
//! \file stats/diagnormal.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "multivariate.hpp"

namespace stateline
{
  namespace stats
  {
    class DiagNormal : public Multivariate
    {
      public:
        DiagNormal(const Eigen::VectorXd &mean, const Eigen::VectorXd &cov);

        Eigen::VectorXd mean() const;
        Eigen::VectorXd diag() const;

      private:
        Eigen::VectorXd mean_;
        Eigen::VectorXd diag_;

        friend double logpdf<>(const DiagNormal &d, const Eigen::VectorXd &x);
    };

    template <>
    Eigen::VectorXd mean(const DiagNormal &d);

    template <>
    Eigen::VectorXd var(const DiagNormal &d);

    template <>
    double logpdf(const DiagNormal &d, const Eigen::VectorXd &x);
  }
}
