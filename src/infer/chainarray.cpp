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
    ChainSettings ChainSettings::Default(bool recoverFromDisk)
    {
      ChainSettings settings;
      settings.recoverFromDisk = recoverFromDisk;
      //settings.chainCacheLength = 1000;
      settings.chainCacheLength = 10; // For testing
      return settings;
    }

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


    ChainArray::ChainArray(uint nStacks, uint nChains,
                           const ChainSettings& settings)
        : //db_({settings.databasePath, settings.databaseCacheSizeMB}, settings.recoverFromDisk),
          writer_(".", nStacks),
          nstacks_(nStacks),
          nchains_(nChains),
          cacheLength_(settings.chainCacheLength),
          lengthOnDisk_(nStacks * nChains, 0),
          beta_(nStacks * nChains),
          sigma_(nStacks * nChains),
          cache_(nStacks * nChains)
    {
      std::cout << "size: " << lengthOnDisk_.size() << std::endl;
      for (uint i=0; i < nstacks_*nchains_; i++)
        cache_[i].reserve(cacheLength_);

      if (settings.recoverFromDisk)
        recover();
      else
        init();
    }

    /*
    ChainArray::ChainArray(ChainArray&& other)
      :*db_(std::move(db_)), nstacks_(other.nstacks_), nchains_(other.nchains_),
      cacheLength_(other.cacheLength_), beta_(std::move(other.beta_)), sigma_(std::move(other.sigma_)),
      cache_(std::move(other.cache_))
    {
    }*/

    ChainArray::~ChainArray()
    {
      for (uint i = 0; i < nstacks_*nchains_; i++)
      {
        if (cache_[i].size() > 0)
          flushToDisk(i);
      }
    }
    
    
    void ChainArray::recover()
    {
      CHECK(false) << "Recovery not implemented";
      /*
      // Recover the dimensions of the chain array
      nstacks_ = detail::getFromDb<detail::NSTACKS, std::uint32_t>(db_);
      nchains_ = detail::getFromDb<detail::NCHAINS, std::uint32_t>(db_);

      // Recover each of the chains
      for (uint id = 0; id < nstacks_ * nchains_; ++id)
      {
        // Recover beta and sigma
        VLOG(1) << "Recovering chain " << id << " from disk.";
        beta_[id] = detail::getFromDb<detail::BETA, double>(db_, id);
        beta_[id] = detail::getFromDb<detail::SIGMA, double>(db_, id);
      }*/
    }

    void ChainArray::init()
    {
      /*
      // Initialise the database
      leveldb::WriteBatch batch;
      detail::putToBatch<detail::NSTACKS, std::uint32_t>(batch, 0, 0, nstacks_);
      detail::putToBatch<detail::NCHAINS, std::uint32_t>(batch, 0, 0, nchains_);
      for (uint i = 0; i < nstacks_*nchains_; i++)
        detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, i, 0, 0);
      db_.put(batch);*/
    }

    
    //uint ChainArray::lengthOnDisk(uint id) const
    //{
      //return detail::getFromDb<detail::LENGTH, std::uint32_t>(db_, id);
      //return lengthOnDisk_[id];
    //}

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
      if (cache_[id].size() == cacheLength_)
        flushToDisk(id);

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
      uint diskLength = lengthOnDisk_[id];
      uint cacheLength = cache_[id].size();

      // for t=1 chains only (cold) chains only
      if (chainIndex(id) == 0)
      {
        uint newLength = diskLength + cacheLength;
        std::cout << "flushing cache " << newLength << std::endl;
        VLOG(3) << "Flushing cache of chain " << id << ". new length on disk: " << newLength;
        std::vector<State> statesToBeSaved(std::begin(cache_[id]), std::end(cache_[id]));
        writer_.append(id, statesToBeSaved);
        lengthOnDisk_[id] = newLength;
      }

      // Re-initialise the cache
      cache_[id].clear();
    }

    State ChainArray::lastState(uint id) const
    {
      uint idx = lengthOnDisk_[id] + cache_[id].size() - 1;
      std::cout << "length on disk = " << lengthOnDisk_[id] << std::endl;
      return state(id, idx);
    }

    State ChainArray::stateFromDisk(uint id, uint idx) const
    {
      /*
      uint dlen = lengthOnDisk(id);
      CHECK(idx < dlen) << "Can't access state " << idx << " in chain " << id << " from disk when " << dlen << " states stored on disk";
      // Read the serialised state from disk
      return detail::unarchiveString<State>(state);*/
    }

    State ChainArray::stateFromCache(uint id, uint idx) const
    {
      uint dlen = lengthOnDisk_[id];
      uint cacheIdx = idx - dlen;
      CHECK(cacheIdx < cache_[id].size()) << "Can't access state " << idx << " in chain " << id << ": index beyond cache boundary";
      CHECK(cacheIdx >= 0) << "Can't access state " << idx << " in chain " << id << ": negative index into cache";
      return cache_[id][cacheIdx];
    }

    State ChainArray::state(uint id, uint idx) const
    {
      uint dlen = lengthOnDisk_[id];
      if (idx < dlen)
        return stateFromDisk(id, idx);
      else
        return stateFromCache(id, idx);
    }

    std::vector<State> ChainArray::states(uint id, uint nburn, uint nthin) const
    {
      uint len = length(id);

      std::vector<State> v;
      v.reserve((len - nburn) / (nthin + 1) + 1);

      for (uint i = nburn; i < len; i += nthin + 1)
      {
        v.push_back(state(id, i));
      }

      return v;
    }
    
    void ChainArray::setLastState(uint id, const State& state)
    {
      if (cache_[id].size() > 0)
      {
        cache_[id].back() = state;
      }
      else
      {
        db_.replaceLast(id, state);
      }
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
