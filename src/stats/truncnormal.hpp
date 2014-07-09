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

        friend double logpdf<>(const TruncNormal &d, const Eigen::VectorXd &x);
    };

    template <>
    Eigen::VectorXd mean(const TruncNormal &d);

    template <>
    Eigen::MatrixXd cov(const TruncNormal &d);

    template <>
    bool insupport(const TruncNormal &d, const Eigen::VectorXd &x);

    template <>
    double logpdf(const TruncNormal &d, const Eigen::VectorXd &x);
  }
}
