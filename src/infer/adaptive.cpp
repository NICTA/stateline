//!
//! Contains the implementation of adaptors for sigma and beta
//!
//! \file infer/adaptive.cpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/adaptive.hpp"
#include <iostream>
#include "../app/jsonsettings.hpp"

namespace stateline
{
  namespace mcmc
  {

    RegressionAdapter::RegressionAdapter(uint nStacks, uint nTemps, double optimalRate)
      : nStacks_(nStacks), nTemps_(nTemps), optimalRate_(optimalRate),
      estimates_(nStacks*nTemps), rates_(nStacks*nTemps)
    {}

    void RegressionAdapter::update(uint chainID, const State& state)
    {
    
    }

    const std::vector<double>& RegressionAdapter::estimates() const
    {
      return estimates_;
    }

    const std::vector<double>& RegressionAdapter::rates() const
    {
      return rates_;
    }

    CovarianceEstimator::CovarianceEstimator(uint nStacks, uint nTemps, uint nDims)
      : lengths_(nStacks * nTemps, 0),
      covs_(nStacks * nTemps, Eigen::MatrixXd::Identity(nDims, nDims)),
      a_(nStacks * nTemps, Eigen::MatrixXd::Identity(nDims, nDims)),
      u_(nStacks * nTemps, Eigen::VectorXd::Zero(nDims))
    {
    }

    void CovarianceEstimator::update(uint i, const Eigen::VectorXd &sample)
    {
      // 10 is theoretically optimal
      double n = (double)lengths_[i] + 10 * sample.size();

      a_[i] = a_[i] * (n / (n + 1)) + (sample * sample.transpose()) / (n + 1);
      u_[i] = u_[i] * (n / (n + 1)) + sample / (n + 1);

      covs_[i] = a_[i] - (u_[i] * u_[i].transpose());// / (n + 1);

      lengths_[i]++;
    }

    const std::vector<Eigen::MatrixXd> &CovarianceEstimator::covariances() const
    {
      return covs_;
    }
    
  } // mcmc
} // stateline
