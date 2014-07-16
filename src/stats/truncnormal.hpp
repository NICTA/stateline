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

        Eigen::VectorXd min() const;
        Eigen::VectorXd max() const;

      private:
        Eigen::VectorXd min_;
        Eigen::VectorXd max_;

        friend double logpdf<>(const TruncNormal &d, const Eigen::VectorXd &x);

        template <class RNG>
        friend Eigen::VectorXd sample(const TruncNormal &d, RNG &rng);
    };

    template <>
    Eigen::VectorXd mean(const TruncNormal &d);

    template <>
    Eigen::MatrixXd cov(const TruncNormal &d);

    template <>
    bool insupport(const TruncNormal &d, const Eigen::VectorXd &x);

    template <>
    double logpdf(const TruncNormal &d, const Eigen::VectorXd &x);

    template <class RNG>
    Eigen::VectorXd sample(const TruncNormal &d, RNG &rng)
    {
      // Perform rejection sampling to ensure that the sample is within
      // the support of the distribution.
      const Normal &base = *static_cast<const Normal *>(&d);

      Eigen::VectorXd result(length(d));

      // In case the bounds are too tight, we set a maximum number of attempts
      // before throwing an exception.
      int attemptsLeft = 100000;
      do
      {
        // Get a sample from the non-truncated normal distribution.
        result = sample(base, rng);
        --attemptsLeft;
      } while (attemptsLeft > 0 && !insupport(d, result));

      // Throw an exception if we still haven't gotten a sample
      if (!insupport(d, result)) {
        throw std::runtime_error("Maximum number of rejections reached before "
            "a sample could be generated");
      }

      // Result is in the support
      return result;
    }
  }
}
