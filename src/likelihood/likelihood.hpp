//!
//! Contains the interface for common likelihood models.
//!
//! \file stats/likelihood.hpp
//! \author Darren Shen
//! \date 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>

namespace stateline
{
  namespace lh
  {
    //! Calculate the normal log likelihood.
    //! 
    //! \param x The sample vector.
    //! \param mean The mean vector of the distribution.
    //! \param std The standard deviation of the distribution.
    //!
    double normal(const Eigen::VectorXd &x, const Eigen::VectorXd &candidate, double std);

    //! Calculate the normal inverse Gamma marginal log likelihood.
    //! 
    //! \param x The sample vector.
    //! \param mean The mean vector of the distribution.
    //! \param A, B Alpha and beta parameters.
    //!
    double normalInverseGamma(const Eigen::VectorXd &x, const Eigen::VectorXd &mean, double A, double B);
  }
}
