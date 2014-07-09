//!
//! Contains the interface for the truncated normal distribution.
//!
//! \file stats/truncnormal.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "stats/multivariate.hpp"
#include "stats/normal.hpp"

namespace stateline
{
  namespace stats
  {
    class TruncNormal : public Normal
    {
      public:
        TruncNormal(const Eigen::VectorXd &mean, const Eigen::MatrixXd &cov,
            const Eigen::VectorXd &min, const Eigen::VectorXd &max);

        Eigen::VectorXd &min() const;
        Eigen::VectorXd &max() const;

      private:
        Eigen::VectorXd min_;
        Eigen::VectorXd max_;

        // TODO: urgh
        friend double ulogpdf<>(const TruncNormal &d, const Eigen::VectorXd &x);
    };

    template <>
    bool insupport(const TruncNormal &d, const Eigen::VectorXd &x);

    // TODO: currently logpdf is deprecated because the normalisation factor
    // is hard to calculate.
    template <>
    double logpdf(const TruncNormal &d, const Eigen::VectorXd &x);

    template <>
    double ulogpdf(const TruncNormal &d, const Eigen::VectorXd &x);
  }
}
