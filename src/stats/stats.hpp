//!
//! Contains the interface for various statistical functions.
//!
//! \file distrib/stats.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <cassert>
#include <cmath>
#include <Eigen/Dense>

namespace stateline
{
  namespace stats
  {
    //! Return the number of dimensions in a distribution.
    template <class Distribution>
    std::size_t length(const Distribution &d)
    {
      return d.length();
    }

    //! Return the mean vector of a distribution.
    template <class Distribution>
    Eigen::VectorXd mean(const Distribution &d);

    //! Return the variance of a distribution.
    template <class Distribution>
    Eigen::VectorXd var(const Distribution &d)
    {
      // By default, use the diagonal of the covariance matrix
      return cov(d).diagonal();
    }

    //! Return the covariance of a distribution.
    template <class Distribution>
    Eigen::MatrixXd cov(const Distribution &d)
    {
      // By default, use the variance as the main diagonal
      return var(d).asDiagonal();
    }

    template <class Distribution>
    bool insupport(const Distribution &d, const Eigen::VectorXd &x)
    {
      // Assume that distributions are not bounded by default
      return true;
    }

    //! Evaluate an unormalised PDF of a distribution. 
    template <class Distribution>
    double pdf(const Distribution &d, const Eigen::VectorXd &x)
    {
      return std::exp(logpdf(d, x));
    }

    //! Evaluate an unnormalised log PDF of a distribution.
    template <class Distribution>
    double logpdf(const Distribution &d, const Eigen::VectorXd &x)
    {
      return std::log(pdf(d, x));
    }

    //! Evaluate an unnormalised negative log PDF of a distribution.
    template <class Distribution>
    double nlogpdf(const Distribution &d, const Eigen::VectorXd &x)
    {
      return -logpdf(d, x);
    }

    //! Draw a random sample from a distribution
    template <class Distribution, class RNG>
    Eigen::VectorXd sample(const Distribution &d, RNG &rng);
  }
}
