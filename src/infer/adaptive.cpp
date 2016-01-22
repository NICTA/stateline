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
    // Sane log max and min values for safety
    // They also control the initial guess
    const double min_logsigma_ = -10.;
    const double max_logsigma_ = 10.;
    const double log_beta_factor_ = 0.;  // the min and max were selected for sigma
    const double initial_count_ = 50.;  // controlls convergence rate
    const double temp_variance_ = 10.;  // Initial guess of temp variance
    const uint n_window_ = 1000;  // length of logging window

    RegressionAdapter::RegressionAdapter(uint nStacks, uint nTemps, double optimalRate)
      : nStacks_(nStacks), nTemps_(nTemps), optimalRate_(optimalRate),
      mu_xy_(nTemps), mu_xx_(nTemps), weight_(nTemps), count_(nTemps),
      window_(nTemps*nStacks), window_sum_(nTemps*nStacks, 0), 
      rates_(nStacks*nTemps, 0./0.), values_(nStacks*nTemps, 1.)
    {
      // Initialial output is 1.0.
      // Intial count is nonzero for stability with few samples
      Eigen::Vector3d bound1(-max_logsigma_, 0., 1.);
      Eigen::Vector3d bound2(-min_logsigma_, 0., 1.);

      Eigen::Matrix3d mu_xx =  0.5 * bound1 * bound1.transpose() +
          0.5 * bound2 * bound2.transpose();
      
      Eigen::Vector3d mu_xy = 0.5*bound2;

      for (uint t=0; t<nTemps; t++)
      {
        mu_xy_[t] = mu_xy;
        mu_xx_[t] = mu_xx;
        weight_[t] = mu_xy;
        count_[t] = initial_count_;
      }
      
    }

    // Generic Learner
    void RegressionAdapter::update(uint chainID, double val, double t, bool acc)
    {
        
      // Update regressor weights
      uint tempID = chainID % nTemps_;
      double logval = std::min(std::max(log(val), min_logsigma_), max_logsigma_);  // clip to valid
      Eigen::Vector3d x(-logval, t, 1.);  // 3x1
      double y = acc;  // double version of accept
      count_[tempID] ++;  // already type double
      double alpha = 1. / count_[tempID];  // compute this way so it goes to 0
      mu_xx_[tempID] = mu_xx_[tempID] * (1. - alpha) + x * x.transpose() * alpha;
      Eigen::Vector3d &mu_xy = mu_xy_[tempID];
      mu_xy = mu_xy * (1. - alpha) + x * y * alpha;
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
      const double eps = 1e-3;  // min gradient of this parameter with accept rate
      double denom = std::max(eps, W[0]);
      double numer = -(optimalRate_ - W[1]*t - W[2]);
      numer = std::max(std::min(numer, denom*max_logsigma_), denom*min_logsigma_);
      double x = numer / denom;

      // check for NaNs (hopefully never...):
      if (x != x)
      {
        std::cout << "\nChain " << chainID << " has NaN prediction weights " << W.transpose() << "\n";
        std::cout << mu_xx_[tempID] << "\n";
        std::cout << "And covariance:\n";
        std::cout << mu_xy_[tempID].transpose() << "\n";
        throw 15;
      }

      return x;  // convert from log-space in calling function
    }

    void RegressionAdapter::betaUpdate(uint chainID, double bl, double bh, bool acc)
    {

      // Forward transform is:
      // temp(t+1) = (1. + predict(i, temp(t))) * temp(t)

      // Inverse transform is:
      // predict(t, temp(t)) = temp(t+1)/temp(t) - 1. = beta(t)/beta(t+1) - 1.
      
      // Therefore learning step is:
      double target = (bl/bh - 1.) / exp(log_beta_factor_);
      target = std::min(std::max(target, exp(min_logsigma_)), exp(max_logsigma_));
      update(chainID, target, -log(bl), acc);
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
        double logfactor = predict(i-1, log(temp)) + log_beta_factor_;
        temp *= (1. + exp(logfactor));  // ratio of 2 initially...
        /* temp *= 3; // for now... */
        values_[chainID+i] = 1./temp;          
      }
    }

    double RegressionAdapter::computeSigma(uint chainID, double t)
    {
      double sigma = exp(predict(chainID, t));
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
