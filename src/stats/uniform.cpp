//!
//! Contains the implementation for the uniform distribution.
//!
//! \file stats/uniform.cpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/uniform.hpp"

#include <random>

namespace stateline
{
  namespace stats
  {
    Uniform::Uniform(const Eigen::VectorXd &min, const Eigen::VectorXd &max)
      : Multivariate(min.size()),
        min_(min), max_(max)
    {
      assert(min.size() == max.size());
    }

    Eigen::VectorXd Uniform::min() const
    {
      return min_;
    }

    Eigen::VectorXd Uniform::max() const
    {
      return max_;
    }

    template <>
    Eigen::VectorXd mean(const Uniform &d)
    {
      return 0.5 * (d.min() + d.max());
    }

    template <>
    Eigen::VectorXd var(const Uniform &d)
    {
      return 1.0 / 12.0 * (d.max() - d.min());
    }

    template <>
    bool insupport(const Uniform &d, const Eigen::VectorXd &x)
    {
      // Check that all dimensions are within the bounds of the distribution.
      return ((d.min().array() < x.array()) && (x.array() < d.max().array())).all();
    }

    template <>
    double logpdf(const Uniform &d, const Eigen::VectorXd &x)
    {
      // Check if x is in the support of the distribution
      if (insupport(d, x))
      {
        return 0.0;
      }
      else
      {
        return -std::numeric_limits<double>::infinity();
      }
    }

    template <class RNG>
    Eigen::VectorXd sample(const Uniform &d, RNG &rng)
    {
      std::uniform_real_distribution<double> dist(0, 1);

      Eigen::ArrayXd sample(length(d));
      for (Eigen::ArrayXd::Index i = 0; i < sample.rows(); i++)
      {
        sample(i) = dist(rng);
      }

      // Transform the output to fit within the support of the distribution
      return d.min().array() + sample * (d.max() - d.min()).array();
    }
  }
}
