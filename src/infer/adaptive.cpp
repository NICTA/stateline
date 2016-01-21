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
      mu_xy_(nTemps), mu_xx_(nTemps), weight_(nTemps), count_(nTemps),
      window_(nTemps*nStacks), window_sum_(nTemps*nStacks, 0), 
      rates_(nStacks*nTemps, optimalRate), values_(nStacks*nTemps, 1.)
    {
      // Initialial output is 1.0.
      // Intial count is nonzero for stability with few samples
      double initial_count=10.;
      Eigen::Vector3d mu_xy(1., 0., optimalRate_); 
      for (uint t=0; t<nTemps; t++)
      {
        mu_xy_[t] = mu_xy;
        mu_xx_[t].setIdentity();
        weight_[t] = mu_xy;
        count_[t] = initial_count;
      }
      
    }

    // Generic Learner
    void RegressionAdapter::update(uint chainID, double val, double t, bool acc)
    {
      // Update regressor weights
      uint tempID = chainID % nTemps_;
      Eigen::Vector3d x(-log(val), t, 1.);  // 3x1
      double y = acc;
      count_[tempID] ++;
      double alpha = 1. / count_[tempID];  // compute this way so it goes to 0
      mu_xx_[tempID] = mu_xx_[tempID] * (1. - alpha) + x * x.transpose() * alpha;
      mu_xy_[tempID] = mu_xy_[tempID] * (1. - alpha) + x * y * alpha;
      weight_[tempID] = mu_xx_[tempID].colPivHouseholderQr().solve(mu_xy_[tempID]);

      // Update the accept rate logging
      int ia = acc;  // int version of accepted
      window_[chainID].push_back(ia);
      window_sum_[chainID] += ia;
      uint n = window_[chainID].size();
      if (n >= n_window_)
      {
        window_sum_[chainID] -= window_[chainID][0];
        window_[chainID].pop_front();
      }
      rates_[chainID] = (double) window_sum_[chainID] / (double) n;
    }

    // Generic predictor
    double RegressionAdapter::predict( uint chainID, double t) const
    {
      // Invert the linear model for tempID
      uint tempID = chainID % nTemps_;
      const Eigen::Vector3d &W = weight_[tempID];
      double x = -(optimalRate_ - W[1]*t - W[2]) / W[0];
      x = std::max(std::min(x, max_logsigma_), min_logsigma_);
      return exp(x);  // convert from log-space
    }

    void RegressionAdapter::betaUpdate(uint chainID, double bl, double bh, bool acc)
    {

      // Forward transform is:
      // temp(t+1) = (1. + predict(i, temp(t))) * temp(t)

      // Inverse transform is:
      // predict(t, temp(t)) = temp(t+1)/temp(t) - 1. = beta(t)/beta(t+1) - 1.
      
      // Therefore learning step is:
      update(chainID, bl/bh - 1., 1./bl, acc);
    }

    void RegressionAdapter::computeBetaStack(uint chainID)
    {
      // Queries the model for an entire stack (based on the chain ID)
      // chainID is the coldest chain in its stack
      // Results are cached in values_
      
      // t(i+1) = (1. + predict(i, t(i))) * t(i)

      double temp = 1.;
      for (uint i=1; i<nTemps_; i++)
      {
        // Note, chainID + i-1 % nTemps = i-1
        temp = (1. + predict(i-1, temp));  // ratio of 2 initially...
        values_[chainID+i] = 1./temp;          
      }
    }

    double RegressionAdapter::computeSigma(uint chainID, double t)
    {
      double sigma = predict(chainID, t);
      values_[chainID] = sigma;  // save the last value used.
      return sigma;
    }

    // For logging and remembering the last used values
    const std::vector<double>& RegressionAdapter::values() const
    {
      return values_;
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
