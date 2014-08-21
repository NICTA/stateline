//!
//! Contains the implementation of some statistical diagnostic tests for MCMC.
//!
//! \file infer/diagnostics.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>

#include "state.hpp"

namespace stateline
{
  namespace mcmc
  {
    //! Convergence test using estimated potential scale reduction (EPSR).
    //! It is a convergence metric that takes into account the variance of the 
    //! means between chains and the variance of the samples within each chain.
    //!
    class EPSRDiagnostic
    {
      public:
        //! Initialise the convergence criteria.
        //!
        //! \param nStacks The number of stacks.
        //! \param nChains The number of stacks per chain.
        //! \param numDims The number of dimensions in each state.
        //! \param threshold Threshold for convergence.
        //!
        EPSRDiagnostic(uint nStacks, uint nChains, uint nDims, double threshold = 1.1) :
          nStacks_(nStacks),
          nChains_(nChains),
          M_(Eigen::ArrayXXd::Zero(nDims, nStacks)),
          S_(Eigen::ArrayXXd::Zero(nDims, nStacks)),
          numSamples_(Eigen::ArrayXi::Zero(nStacks)),
          threshold_(threshold)
        {
        }

        //! Update the convergence statistics for a new sample in a particular chain.
        //!
        //! \param id The chain which has the new sample.
        //! \param sample The new sample to update the convergence statistics with.
        //!
        void update(uint id, const State& state)
        {
          // We only monitor the convergence of the coldest chains
          if (id % nChains_ == 0)
          {
            // Convert global ID to coldest chain ID
            id = id / nChains_;

            // See http://www.johndcook.com/standard_deviation.html
            int n = numSamples_(id) + 1;

            Eigen::ArrayXd x = state.sample.array();

            // Update the running mean and variance
            Eigen::ArrayXd newM = M_.col(id) + (x - M_.col(id)) / n;
            S_.col(id) = S_.col(id) + (x - M_.col(id)) * (x - newM);
            M_.col(id) = newM;

            numSamples_(id) = n;
          }
        }

        //! Compute the estimated potential scale factor. A low value indicates
        //! that the chains are converging.
        //!
        //! \return A vector containing the scale factors for each dimension.
        //!
        Eigen::ArrayXd rHat() const
        {
          // Number of samples (use the length of the shortest chain)
          int n = numSamples_.minCoeff();

          // Number of chains in the coldest temperature
          int m = nStacks_;

          // Find the overall mean of the chains
          Eigen::ArrayXXd overallMean = M_.rowwise().mean().replicate(1, M_.cols());

          // Calculate the between chain variance
          Eigen::ArrayXd b = (n / (m - 1.0)) * (M_ - overallMean).matrix().rowwise().squaredNorm().array();

          // Calculate the within chain variance
          Eigen::ArrayXd w = ((1.0 / m) * S_ / (n - 1.0)).rowwise().sum();

          // Compute the weighted average of the between chain variance and within chain variance
          Eigen::ArrayXd vHat = ((n - 1.0) / n) * w + (1.0 / n) * b;

          // Compute the potential scale reduction
          return (vHat / (w + 1e-30)).sqrt();
        }

        //! Check if all the chains have converged. The chains have converged if the
        //! potential scale reduction factor is below the threshold for all dimensions.
        //!
        //! \return Whether all the chains have converged.
        //!
        bool hasConverged() const
        {
          return (rHat() < threshold_).all();
        }

      private:
        uint nStacks_;
        uint nChains_;
        Eigen::ArrayXXd M_;
        Eigen::ArrayXXd S_;
        Eigen::ArrayXi numSamples_;
        double threshold_;
    };
  }
}
