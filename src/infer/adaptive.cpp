//!
//! Contains the implementation of adaptors for sigma and beta
//!
//! \file infer/adaptive.cpp
//! \author Darren Shen, Alistair Reid
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/adaptive.hpp"
#include <iostream>
#include "../app/jsonsettings.hpp"
#include <Eigen/Dense>
#include <vector>
#include <deque>
#include <math.h>


namespace stateline
{
  namespace mcmc
  {

    RegressionAdapter::RegressionAdapter(uint nStacks, uint nTemps, double optimalRate)
      : nStacks_(nStacks), nTemps_(nTemps), optimalRate_(optimalRate),
      mu_xy_(nTemps), weight_(nTemps), mu_xx_(nTemps), count_(nTemps),
      window_(nTemps*nStacks), window_sum(nTemps*nStacks, 0), 
      rates_(nStacks*nTemps, optimalRate_), 
      estimates_(nStacks*nTemps, 1.)
    {

      // Initialise linear models so initial choice is 1.0.
      // Also, intial count is nonzero for stability with few samples
      double initial_count=10.;
      Eigen::Vector3d mu_xy(1., 0., optimalRate_); 
      for (int t=0; t<nTemps; t++)
      {
        mu_xy_[t] = mu_xy;
        mu_xx_[t].setIdentity();
        count_[t] = initial_count;
      }
      
    }

    void RegressionAdapter::update(uint chainID, double sigm, double t, bool acc)
    {

      // Update regressor weights
      uint tempID = chainID % nTemps_;
      Eigen::Vector3d x(-log(sigm), t, 1.);  // 3x1
      double y = acc;
      counts_[tempID] ++;
      double alpha = 1. / counts_[tempID];  // compute this way so it goes to 0
      mu_xx[tempID] = mu_xx[tempID] * (1. - alpha) + x * x.transpose() * alpha;
      mu_xy[tempID] = mu_xy[tempID] * (1. - alpha) + x * y * alpha;
      weights[tempID] = mu_xx[tempID].colPivHouseholderQr().solve(
              mu_xy[tempID]);

      // Update the accept rate logging
      int ia = acc;  // int version of accepted
      window_[chainID].push_back(ia)
      window_sum_[chainID] += ia
      n = window_[chainID].size()
      if (n >= n_window)
      {
        window_sum_[chainID] -= windows[chainID][0]
        window_[chainID].pop_front()
      }
      rates_[chainID] = (double) window_sum_[chainID] / (double) n;
    }

    const std::vector<double>& RegressionAdapter::sigma( uint chainID, double t) const
    {
      // For a given temperature, pick a sigma for this chain ID
      uint tempID = chainID % nTemps_;
      Eigen::Vector3d &W = weights[tempID]

      // Invert the linear model
      double sigma = (optimalRate_ - W[1]*t - W[2]) / W[0];
      sigma = std::min(std::max(v, -max_logsigma), -min_logsigma);
      sigma = np.exp(-sigma);  // compute sigma

      // lazy logging for sigma - it will be the last one used...
      estimates_[chainID] = sigma;

      return sigma;  // actual sigma
    }

    // For logging and remembering the last used values
    const std::vector<double>& RegressionAdapter::estimates() const
    {
      return estimates_;
    }

    // For logging only
    const std::vector<double>& RegressionAdapter::rates() const
    {
      return rates_;
    }



    // TODO(Al) Incremental cholesky factor tracking
    CovarianceEstimator::CovarianceEstimator(uint nStacks, uint nTemps, uint nDims)
      : lengths_(nStacks * nTemps, 0),
      covs_(nStacks * nTemps, Eigen::MatrixXd::Identity(nDims, nDims)),
      a_(nStacks * nTemps, Eigen::MatrixXd::Identity(nDims, nDims)),
      u_(nStacks * nTemps, Eigen::VectorXd::Zero(nDims))
    {
    }

    void CovarianceEstimator::update(uint i, const Eigen::VectorXd &sample)
    {
      // TODO(Al): we should be estimating the cholesky decomposition online.
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
