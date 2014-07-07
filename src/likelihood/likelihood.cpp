//!
//! \file likelihood/likelihood.cpp
//! \author Darren Shen
//! \date May 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "likelihood.hpp"

#include <glog/logging.h>
#include <cmath>

namespace obsidian
{
  namespace lh
  {
    double stdDev(const Eigen::VectorXd& x)
    {
      return std::sqrt((x.array() - x.mean()).pow(2.0).sum() / (double) (x.size()));
    }

    double stdDev(const std::vector<Eigen::VectorXd>& x)
    {
      uint totalSize = 0;
      for (auto const& i : x)
        totalSize += i.size();
      Eigen::VectorXd full(totalSize);
      uint start = 0;
      for (auto const& i : x)
      {
        full.segment(start, i.size()) = i;
        start += i.size();
      }
      return stdDev(full);
    }

    double gaussian(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double sensorSd)
    {
      Eigen::VectorXd delta = real - candidate;
      return -delta.squaredNorm() / (2 * sensorSd * sensorSd) - 0.5 * std::log(6.28318530718 * sensorSd * sensorSd) * delta.rows();
    }

    double normalInverseGamma(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double A, double B)
    {
      Eigen::VectorXd delta = real - candidate;

      double norm = std::lgamma(A + 0.5) - std::lgamma(A) - 0.5 * std::log(6.28318530718) + std::log(B) * A;
      return (-(A + 0.5) * (B + 0.5 * delta.array().square()).log() + norm).sum();
    }
  }
}
