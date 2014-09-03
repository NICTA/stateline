//!
//! Contains the implementation of some statistical diagnostic tests for MCMC.
//!
//! \file infer/diagnostics.cpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "diagnostics.hpp"

namespace stateline
{
  namespace mcmc
  {
    EPSRDiagnostic::EPSRDiagnostic(uint nStacks, uint nChains, uint nDims, double threshold) :
      nStacks_(nStacks),
      nChains_(nChains),
      M_(Eigen::ArrayXXd::Zero(nDims, nStacks)),
      S_(Eigen::ArrayXXd::Zero(nDims, nStacks)),
      numSamples_(Eigen::ArrayXi::Zero(nStacks)),
      threshold_(threshold)
    {
    }

    void EPSRDiagnostic::update(uint id, const State& state)
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

    Eigen::ArrayXd EPSRDiagnostic::rHat() const
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

    bool EPSRDiagnostic::hasConverged() const
    {
      return (rHat() < threshold_).all();
    }
  }
}
