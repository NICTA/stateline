//!
//! Contains the implementation of adaptors for sigma and beta
//!
//! \file infer/adaptive.cpp
//! \author Alistair Reid
//! \author Darren Shen
//! \date 2016
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

    // Not neccessary for the user to tune these...
    const double initial_count_ = 10.;  // To help initial stability 
    const double temp_variance_ = 10.;  // Initial guess of log-beta's variance
    const uint n_window_ = 1000;  // logging window (doesnt affect adaption)

    RegressionAdapter::RegressionAdapter(uint nStacks, uint nTemps, 
            double optimalRate, double min_cap, double max_cap)
      : nStacks_(nStacks), nTemps_(nTemps), min_cap_(min_cap), 
      max_cap_(max_cap), optimalRate_(optimalRate),
      mu_xy_(nTemps), mu_xx_(nTemps), weight_(nTemps), count_(nTemps),
      window_(nTemps*nStacks), window_sum_(nTemps*nStacks, 0), 
      rates_(nStacks*nTemps, 0.), values_(nStacks*nTemps, 1.)
    {
      // Initialialise a valid configuration
      // data-point = (logx, log_sidedata, bias=1.)
      Eigen::Vector3d bound_rej(min_cap, 0., 1.); // max should reject
      Eigen::Vector3d bound_acc(max_cap, 0., 1.); //min should accept

      // Initial guess of input covariance matrix based on 50% accept
      Eigen::Matrix3d mu_xx =  0.5 * bound_rej * bound_rej.transpose() +
          0.5 * bound_acc * bound_acc.transpose();
      mu_xx(1,1) = temp_variance_;  // set nonzero temperature variance

      // initial guess of input/accept covariance consistent with above
      Eigen::Vector3d mu_xy = 0.5*bound_acc;

      // Set as model for all temperatures
      for (uint t=0; t<nTemps; t++)
      {
        mu_xy_[t] = mu_xy;
        mu_xx_[t] = mu_xx;
        weight_[t] = mu_xy;
        count_[t] = initial_count_;
      }
      
    }

    void RegressionAdapter::update(uint chainID, double logval, double side_data, bool acc)
    {
        
      // Update regressor weights
      uint tempID = chainID % nTemps_;
      logval = std::min(std::max(logval, min_cap_), max_cap_);
      Eigen::Vector3d x(-logval, side_data, 1.);  // 3x1
      double y = acc;  // accepted as a double
      count_[tempID] += 1.;
      double alpha = 1. / count_[tempID];
      mu_xx_[tempID] = mu_xx_[tempID] * (1. - alpha) + x * x.transpose() * alpha;
      Eigen::Vector3d &mu_xy = mu_xy_[tempID];
      mu_xy = mu_xy * (1. - alpha) + x * y * alpha;
      // technically we could use the covariance updater below... 
      // but at 3x3, its hardly worth it.
      weight_[tempID] = mu_xx_[tempID].colPivHouseholderQr().solve(mu_xy_[tempID]);

      // Logging: update the accept rate using a circular buffer
      int ia = acc;  // accepted as an int
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
    double RegressionAdapter::predict( uint chainID, double side_data) const
    {
      // Invert the linear model for tempID
      uint tempID = chainID % nTemps_;
      const Eigen::Vector3d &W = weight_[tempID];
      const double eps = 1e-3;  // min gradient of this parameter with accept rate
      double denom = std::max(eps, W[0]);

      // Apply the limiting BEFORE the division to avoid precision issues.
      double numer = -(optimalRate_ - W[1]*side_data - W[2]);
      numer = std::max(std::min(numer, denom*max_cap_), denom*min_cap_);
      return numer / denom;
    }

    void RegressionAdapter::betaUpdate(uint chainID, double bl, double bh, bool acc)
    {
      // Forward transform is:
      // temp(t+1) = predict(i, temp(t)) * temp(t)

      // Inverse transform is:
      // predict(t, temp(t)) = temp(t+1)/temp(t) = beta(t)/beta(t+1) 
      update(chainID, log(bl) - log(bh), log(bl), acc);
    }

    void RegressionAdapter::computeBetaStack(uint chainID)
    {
      // Queries the model for an entire stack (based on the chain ID)
      // chainID is the coldest chain in its stack
      // Results are cached in values_
      // t(i+1) = predict(i, t(i)) * t(i)
      // Note the min log-ratio will be set to zero so temperatures increase
      double logbeta = 0.;
      for (uint i=1; i<nTemps_; i++)
      {
        // Note: (chainID + i-1) % nTemps = i-1
        logbeta -= predict(i-1, logbeta);
        values_[chainID+i] = exp(logbeta);
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



    ProposalShaper::ProposalShaper(uint nStacks, uint nTemps, 
           uint nDims, ProposalBounds bounds, uint initial_count)
      : nDims_(nDims),
      count_(nStacks * nTemps, initial_count),
      L_(nStacks * nTemps, Eigen::MatrixXd::Zero(nDims, nDims)),
      N_(nStacks * nTemps, Eigen::MatrixXd::Zero(nDims, nDims))
    {
        // automatically seed diagonals in proportion to range
        Eigen::VectorXd range = (bounds.max - bounds.min) / 4.;
        prop_norm_ = range.norm();  // save for later scaling
        Eigen::VectorXd rnorm = range / range.norm();
        for (uint i=0; i<nStacks*nTemps; i++)
        {
            L_[i].diagonal() = range;
            N_[i].diagonal() = rnorm;
        }
    }

    void ProposalShaper::update(uint i, const Eigen::VectorXd &sample)
    {
      // As Al prototyped in python...
      const double eps = 1e-18;
      count_[i] ++;
      Eigen::MatrixXd &R = L_[i];
      
      // Handle the incremental scaling
      Eigen::VectorXd x = sample/std::sqrt(count_[i]);
      R *= std::sqrt((count_[i]-1.)/count_[i]);
      
      // And update the estimator cholesky directly
      for (int k=0; k<nDims_; k++)
      {
        double V = std::max(eps, R(k, k));
        double r = std::sqrt(V*V + x[k]*x[k]);
        double c = r / V;
        double s = x[k] / V;
        R(k, k) = r;
        for (int j=k+1; j<nDims_; j++)
        {
          R(j, k) = (R(j, k) + s*x[j]) / c;
          x[j] = c*x[j] - s*R(j, k);
        }
      }

      // Cache a normalised version of this Gaussian...
      N_[i] = R * (prop_norm_ / std::max(R.norm(), eps));

      // Protection from numerical low rank proposals.
      const double min_rank = 1e-4;
      N_[i].diagonal().array() += min_rank;  // Dan says: What could go wrong?
      /* Eigen::VectorXd diag = N_[i].diagonal(); */
      /* for (int i=0; i< nDims_; i++) */
      /*   diag[i] += min_rank; */

    }

    const std::vector<Eigen::MatrixXd> &ProposalShaper::Ns() const
    {
      return N_;
    }
    
  } // mcmc
} // stateline
