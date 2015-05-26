//!
//! Contains the interface of some statistical diagnostic tests for MCMC.
//!
//! \file infer/diagnostics.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>

#include "../infer/datatypes.hpp"

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
        EPSRDiagnostic(uint nStacks, uint nChains, uint nDims, double threshold = 1.1);

        //! Update the convergence statistics for a new sample in a particular chain.
        //!
        //! \param id The chain which has the new sample.
        //! \param sample The new sample to update the convergence statistics with.
        //!
        void update(uint id, const State& state);

        //! Compute the estimated potential scale factor. A low value indicates
        //! that the chains are converging.
        //!
        //! \return A vector containing the scale factors for each dimension.
        //!
        Eigen::ArrayXd rHat() const;

        //! Check if all the chains have converged. The chains have converged if the
        //! potential scale reduction factor is below the threshold for all dimensions.
        //!
        //! \return Whether all the chains have converged.
        //!
        bool hasConverged() const;

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
