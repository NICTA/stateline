//!
//! Contains the interface for normal distributions with diagonal covariance
//! matrix.
//!
//! \file stats/diagnormal.hpp
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
    class DiagNormal : Multivariate
    {
      public:
        DiagNormal(const Eigen::VectorXd &mean, const Eigen::VectorXd &cov);

        Eigen::VectorXd mean() const;
        Eigen::VectorXd diag() const;

      private:
        Eigen::VectorXd mean_;
        Eigen::VectorXd diag_;

        // Normalisation factor
        double norm_;

        friend double logpdf<>(const DiagNormal &d, const Eigen::VectorXd &x);
    };

    template <>
    Eigen::VectorXd var(const DiagNormal &d);

    template <>
    double logpdf(const DiagNormal &d, const Eigen::VectorXd &x);
  }
}
