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

#include <iostream>
#include <easylogging/easylogging++.h>
#include <random>

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


    ChainArray::ChainArray(uint nStacks, uint nTemps, const std::string& outputPath)
        : writer_(outputPath, nStacks),
          nstacks_(nStacks),
          ntemps_(nTemps),
          lengthOnDisk_(nStacks * nTemps, 0),
          beta_(nStacks * nTemps),
          sigma_(nStacks * nTemps),
          cache_(nStacks * nTemps),
          lastFlushTime_(std::chrono::high_resolution_clock::now())
    {
    }

    ChainArray::~ChainArray()
    {
      for (uint i = 0; i < nstacks_*ntemps_; i++)
        flushToDisk(i);
    }

    uint ChainArray::length(uint id) const
    {
      uint result = lengthOnDisk_[id] + cache_[id].size();
      return result;
    }

    bool ChainArray::append(uint id, const Eigen::VectorXd& sample, double energy)
    {
      State newState = {sample, energy, sigma_[id], beta_[id], false, SwapType::NoAttempt};
      State last = lastState(id);
      bool accepted = acceptProposal(newState, last, beta_[id]);

      if (accepted)
        cache_[id].push_back(newState);
      else
        cache_[id].push_back(last);

      cache_[id].back().accepted = accepted;
      cache_[id].back().swapType = SwapType::NoAttempt;
      
      //Flush the chains every so often
      const uint flushTime = 10; //TODO make a setting
      auto now = std::chrono::high_resolution_clock::now();
      uint secs = std::chrono::duration_cast<std::chrono::seconds>(
      now - lastFlushTime_).count();
      if (secs >= flushTime)
      {
        lastFlushTime_ = now;
        for (uint i = 0; i < nstacks_*ntemps_; i++)
          flushToDisk(i);
      }

      return accepted;
    }

    void ChainArray::initialise(uint id, const Eigen::VectorXd& sample, 
        double energy, double sigma, double beta)
    {
      setSigma(id, sigma);
      setBeta(id, beta);
      cache_[id].push_back({ sample, energy, sigma_[id], beta_[id], true, SwapType::NoAttempt});
    }

    void ChainArray::flushToDisk(uint id)
    {
      uint cacheLength = cache_[id].size();
      if (cacheLength == 0)
        return;

      uint diskLength = lengthOnDisk_[id];
      uint newLength = diskLength + cacheLength - 1; //keep one state

      // for t=1 chains only (cold) chains only
      if (chainIndex(id) == 0)
      {
        VLOG(3) << "Flushing cache of chain " << id << ". new length on disk: " << newLength;
        writer_.append(id / numTemps(), std::begin(cache_[id]), 
            std::end(cache_[id])-1);
      }

      // Update length on disk
      lengthOnDisk_[id] = newLength;

      // Clear all but the last element
      mcmc::State recent = cache_[id].back();
      cache_[id].clear();
      cache_[id].push_back(recent);
    }

    const State& ChainArray::lastState(uint id) const
    {
      return cache_[id].back();
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
        std::swap(stateh.sample, statel.sample);
        std::swap(stateh.energy, statel.energy);
        std::swap(stateh.accepted, statel.accepted);
        statel.swapType = SwapType::Accept;
        cache_[hId].back() = stateh;
        cache_[lId].back() = statel;
      }
      else
      {
        statel.swapType = SwapType::Reject;
        cache_[lId].back() = statel;
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

    uint ChainArray::numTemps() const
    {
      return ntemps_;
    }

    uint ChainArray::numTotalChains() const
    {
      return numTemps() * numStacks();
    }

    uint ChainArray::stackIndex(uint id) const
    {
      return id / numTemps();
    }

    uint ChainArray::chainIndex(uint id) const
    {
      return id % numTemps();
    }

    bool ChainArray::isHottestInStack(uint id) const
    {
      return chainIndex(id) == numTemps() - 1;
    }

    bool ChainArray::isColdestInStack(uint id) const
    {
      return chainIndex(id) == 0;
    }

  } // namespace mcmc
} // namespace stateline
