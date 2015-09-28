//!
//! Contains the implementation for common likelihood functions.
//!
//! \file likelihood/likelihood.cpp
//! \author Darren Shen
//! \date 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "likelihood.hpp"

#include <cmath>

namespace stateline
{
  namespace lh
  {
    const double log2PI = std::log(2 * M_PI);

    double normal(const Eigen::VectorXd &x, const Eigen::VectorXd &mean, double std)
    {
      double m = (x - mean).squaredNorm();
      return -0.5 * log2PI - std::log(std) - 1.0 / (2.0 * std * std) * m;
    }

    double normalInverseGamma(const Eigen::VectorXd &x, const Eigen::VectorXd &mean, double A, double B)
    {
      Eigen::VectorXd delta = x - mean;

      double norm = std::lgamma(A + 0.5) - std::lgamma(A) - 0.5 * log2PI + std::log(B) * A;
      return (-(A + 0.5) * (B + 0.5 * delta.array().square()).log() + norm).sum();
    }
  }
}
