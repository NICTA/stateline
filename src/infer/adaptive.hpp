//!
//! Contains the implementation of adaption classes for sigma and beta
//!
//! \file infer/adaptive.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Core>
#include <boost/circular_buffer.hpp>
#include "infer/datatypes.hpp"

namespace stateline
{
  namespace mcmc
  {

    struct SlidingWindowSigmaSettings
    {
      //! The number of states in the sliding window used to calculate the
      //! acceptance rate
      uint windowSize;

      //! The value of sigma for the coldest (beta=1) chain
      double coldSigma;

      //! The geometric factor for each hotter chain's sigma
      double sigmaFactor;

      //! The time constant of the diminishing adaption
      uint adaptionLength;

      //! The number of steps between adaptions
      uint nStepsPerAdapt;

      //! The accept rate being targeted by the adaption of sigma
      double optimalAccept;

      //! The multiplicative rate at which adaption occurs
      double adaptRate;

      //! The minimum multiplicative factor by which sigma can change in a
      //! single adaption
      double minAdaptFactor;
      
      //! The maximum multiplicative factor by which sigma can change in a
      //! single adaption
      double maxAdaptFactor;

      static SlidingWindowSigmaSettings Default();
    
    };

    class SlidingWindowSigmaAdapter
    {
      public:
        SlidingWindowSigmaAdapter( uint nStacks, uint nChains, uint nDims, 
            const SlidingWindowSigmaSettings& settings);

        void update(uint chainID, const State& state);

        std::vector<Eigen::VectorXd> sigmas() const;

        std::vector<double> acceptRates() const;

      private:

        void adaptSigma(uint id);

        uint nStacks_;
        uint nChains_;
        std::vector<boost::circular_buffer<bool>> acceptBuffers_;
        std::vector<Eigen::VectorXd> sigmas_;
        std::vector<double> acceptRates_;
        std::vector<uint> lengths_;
        SlidingWindowSigmaSettings s_;
    };

    struct SlidingWindowBetaSettings
    {
      //! The number of states in the sliding window used to calculate the
      //! swap rate
      uint windowSize;

      //! The geometric factor for each hotter chain's sigma
      double betaFactor;

      //! The time constant of the diminishing adaption
      uint adaptionLength;

      //! The number of steps between adaptions
      uint nStepsPerAdapt;

      //! The swap rate being targeted by the adaption of beta
      double optimalSwapRate;

      //! The multiplicative rate at which adaption occurs
      double adaptRate;

      //! The minimum multiplicative factor by which beta can change in a
      //! single adaption
      double minAdaptFactor;

      //! The maximum multiplicative factor by which beta can change in a
      //! single adaption
      double maxAdaptFactor;

      static SlidingWindowBetaSettings Default();

    };

    class SlidingWindowBetaAdapter
    {
      public:
        
        SlidingWindowBetaAdapter( uint nStacks, uint nChains, 
            const SlidingWindowBetaSettings& settings);
        
        void update(uint id, State s);

        std::vector<double> betas();
        
        std::vector<double> swapRates() const;

      private:
        
        void adaptBeta(uint id);

        uint nStacks_;
        uint nChains_;
        std::vector<double> betas_;
        std::vector<double> swapRates_;
        std::vector<boost::circular_buffer<bool>> swapBuffers_;
        std::vector<uint> lengths_;
        SlidingWindowBetaSettings s_;
    };

    
  }
}
