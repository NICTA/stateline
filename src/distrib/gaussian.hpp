//!
//! Contains the interface for the Gaussian distribution.
//!
//! \file distrib/gaussian.hpp
//! \author Lachlan McCalman
//! \date February 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>
#include <cmath>

namespace obsidian
{
  namespace distrib
  {
    //! Compute the log Gaussian distribution PDF with a diagonal covariance matrix.
    //!
    //! \param x The vector to evaluate the distribution at.
    //! \param mu The mean vector containing the means of each dimension.
    //! \param sigma The diagonal of the covariance matrix.
    //!
    double logGaussian(const Eigen::VectorXd& x, const Eigen::VectorXd& mu, const Eigen::VectorXd& sigma)
    {
      double coeff = -std::log(std::pow(2.0 * M_PI, x.size() * 0.5) * sigma.prod());
      return coeff - 0.5 * (x - mu).cwiseQuotient(sigma).squaredNorm();
    }
  }
}
