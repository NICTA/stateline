//!
//! Contains the implementation of random-walk Metropolis-Hastings sampler.
//!
//! \file infer/metropolis.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/metropolis.hpp"

namespace stateline
{
  namespace mcmc
  {
    bool acceptProposal(const State& newState, const State& oldState, double beta)
    {
      static std::random_device rd;
      static std::mt19937 generator(rd());
      static std::uniform_real_distribution<> rand; // defaults to [0,1)

      if (std::isinf(newState.energy))
        return false;
      double deltaEnergy = newState.energy - oldState.energy;
      double probToAccept = std::min(1.0, std::exp(-1.0 * beta * deltaEnergy));

      // Roll the dice to determine acceptance
      bool accept = rand(generator) < probToAccept;
      return accept;
    }
    
    bool acceptSwap(const State& sL, const State& sH, double betaL, double betaH)
    {
      static std::random_device rd;
      static std::mt19937 generator(rd());
      static std::uniform_real_distribution<> rand; // defaults to [0,1)

      // Compute the probability of swapping
      double deltaEnergy = sH.energy - sL.energy;
      double deltaBeta = betaH - betaL;
      double probToSwap = std::exp(deltaEnergy * deltaBeta);
      bool swapAccepted = rand(generator) < probToSwap;
      return swapAccepted;
    }
  }
}
