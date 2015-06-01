//!
//! Contains the implementation of MCMC chains.
//!
//! \file infer/chainarray.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/chainarray.hpp"

#include <glog/logging.h>
#include <iostream>

namespace stateline
{
  namespace mcmc
  {
    //! Returns true if we want to accept the MCMC step.
    //!
    //! \param newState The proposed state.
    //! \param oldState The current state of the chain.
    //! \param beta The inverse temperature of the chain.
    //! \return True if the proposal was accepted.
    //!
    bool acceptProposal(const State& newState, const State& oldState, double beta)
    {
      static std::random_device rd;
      static std::mt19937 generator(rd());
      static std::uniform_real_distribution<> rand; // defaults to [0,1)

      if (std::isinf(newState.energy))
        return false;

      double deltaEnergy = newState.energy - oldState.energy;
      double probToAccept = std::exp(-1.0 * beta * deltaEnergy);

      // Roll the dice to determine acceptance
      bool accept = rand(generator) < probToAccept;
      return accept;
    }

    //! Returns true if we want to accept the MCMC swap.
    //!
    //! \param stateLow The state of the lower temperature chain.
    //! \param stateHigh The state of the higher temperature chain.
    //! \param betaLow The inverse temperature of the lower temperature chain.
    //! \param betaHigh The inverse temperature of the high temperature chain.
    //! \return True if the swap was accepted.
    //!
    bool acceptSwap(const State& stateLow, const State& stateHigh, double betaLow, double betaHigh)
    {
      static std::random_device rd;
      static std::mt19937 generator(rd());
      static std::uniform_real_distribution<> rand; // defaults to [0,1)

      // Compute the probability of swapping
      double deltaEnergy = stateHigh.energy - stateLow.energy;
      double deltaBeta = betaHigh - betaLow;
      double probToSwap = std::exp(deltaEnergy * deltaBeta);
      bool swapAccepted = rand(generator) < probToSwap;
      return swapAccepted;
    }


    ChainArray::ChainArray(uint nStacks, uint nChains)
        : nstacks_(nStacks),
          nchains_(nChains),
          states_(nStacks * nChains),
          beta_(nStacks * nChains),
          sigma_(nStacks * nChains)
    {
    }

    uint ChainArray::length(uint id) const
    {
      return states_[id].size();
    }

    bool ChainArray::append(uint id, const Eigen::VectorXd& sample, double energy)
    {
      State newState = {sample, energy, sigma_[id], beta_[id], false, SwapType::NoAttempt};
      State last = lastState(id);
      bool accepted = acceptProposal(newState, last, beta_[id]);

      states_[id].push_back(accepted ? newState : last);

      states_[id].back().accepted = accepted;
      states_[id].back().swapType = SwapType::NoAttempt;

      return accepted;
    }

    void ChainArray::initialise(uint id, const Eigen::VectorXd& sample, 
        double energy, double sigma, double beta)
    {
      setSigma(id, sigma);
      setBeta(id, beta);
      states_[id].push_back({ sample, energy, sigma_[id], beta_[id], true, SwapType::NoAttempt});
    }

    State ChainArray::lastState(uint id) const
    {
      return states_[id].back();
    }

    void ChainArray::setLastState(uint id, const State& state)
    {
      states_[id].back() = state;
    }

    SwapType ChainArray::swap(uint id1, uint id2)
    {
      uint hId = std::max(id1, id2);
      uint lId = std::min(id1, id2);

      State stateh = lastState(hId);
      State statel = lastState(lId);

      // Determine if we accept this swap
      bool swapped = acceptSwap(stateh, statel, beta_[hId], beta_[lId]);

      // Save the swap only on the lower temperature chain
      if (swapped)
      {
        std::swap(stateh, statel);
        statel.swapType = SwapType::Accept;
        setLastState(hId, stateh);
        setLastState(lId, statel);
      }
      else
      {
        statel.swapType = SwapType::Reject;
        setLastState(lId, statel);
      }

      return swapped ? SwapType::Accept : SwapType::Reject;
    }

    double ChainArray::sigma(uint id) const
    {
      return sigma_[id];
    }

    void ChainArray::setSigma(uint id, double sigma)
    {
      sigma_[id] = sigma;
    }

    double ChainArray::beta(uint id) const
    {
      return beta_[id];
    }

    void ChainArray::setBeta(uint id, double beta)
    {
      beta_[id] = beta;
    }

    uint ChainArray::numStacks() const
    {
      return nstacks_;
    }

    uint ChainArray::numChains() const
    {
      return nchains_;
    }

    uint ChainArray::numTotalChains() const
    {
      return numChains() * numStacks();
    }

    uint ChainArray::stackIndex(uint id) const
    {
      return id / numChains();
    }

    uint ChainArray::chainIndex(uint id) const
    {
      return id % numChains();
    }

    bool ChainArray::isHottestInStack(uint id) const
    {
      return chainIndex(id) == numChains() - 1;
    }

    bool ChainArray::isColdestInStack(uint id) const
    {
      return chainIndex(id) == 0;
    }

  } // namespace mcmc
} // namespace stateline
