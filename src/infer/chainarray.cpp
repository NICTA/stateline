//!
//! Contains the implementation of MCMC chains.
//!
//! \file infer/chainarray.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/chainarray.hpp"

#include <glog/logging.h>

#include "infer/metropolis.hpp"
#include "serial/mcmc.hpp"

namespace stateline
{
  namespace mcmc
  {
    namespace internal
    {
      std::string toDbString(uint id, uint index, const internal::DbEntryType& t)
      {
        uint idx[3] = { uint(t), id, index };
        uint size = 3 * sizeof(uint) / sizeof(char);
        return std::string((char*) &idx[0], size);
      }

      std::string toDbString(uint id, const internal::DbEntryType& t)
      {
        uint idx[2] = { uint(t), id };
        uint size = 2 * sizeof(uint) / sizeof(char);
        return std::string((char*) &idx[0], size);
      }

      double doubleFromDb(const std::string& data)
      {
        return *((double*) &(data[0]));
      }

      uint uintFromDb(const std::string& data)
      {
        return *((uint*) &(data[0]));
      }

    } // namespace internal

    ChainArray::ChainArray(uint nStacks, uint nChains, double tempFactor, double initialSigma, double sigmaFactor, db::Database& db,
                           uint cacheLength, bool recover)
        : nstacks_(nStacks),
          nchains_(nChains),
          cacheLength_(cacheLength),
          beta_(nStacks * nChains),
          sigma_(nStacks * nChains),
          cache_(nStacks * nChains),
          db_(db)
    {
      // Reserve the cache
      for (auto& c : cache_)
        c.reserve(cacheLength);

      // Set ourselves up in the last state
      if (recover)
      {
        LOG(INFO)<< "Recovering chains...";
        for(uint id = 0; id < nChains * nStacks; id++)
        {
          recoverFromCache(id);
        }
      }
      else
      {
        uint uzero = 0;
        leveldb::WriteBatch batch;
        for (uint i = 0; i < nStacks; i++)
        {
          for (uint j = 0; j < nChains; j++)
          {
            uint id = i * nChains + j;

            double beta = 1.0 / std::pow(tempFactor, j);
            double sigma = initialSigma * std::pow(sigmaFactor, j);
            beta_[id] = beta;
            sigma_[id] = sigma;

            // All chains start off with length 0
            batch.Put(internal::toDbString(id, internal::DbEntryType::LENGTH),
                leveldb::Slice((char*)&uzero, sizeof(uint)/sizeof(char)));

            // All chains start off with same initial sigma
            batch.Put(internal::toDbString(id, internal::DbEntryType::SIGMA),
                leveldb::Slice((char*)&sigma, sizeof(double)/sizeof(char)));

            // Temperatures start off as given by the factors
            batch.Put(internal::toDbString(id, internal::DbEntryType::BETA),
                leveldb::Slice((char*)&beta, sizeof(double)/sizeof(char)));
          }
        }
        db_.batch(batch);
      }
    }

    uint ChainArray::lengthOnDisk(uint id)
    {
      return internal::uintFromDb(db_.get(internal::toDbString(id, internal::DbEntryType::LENGTH)));
    }

    uint ChainArray::length(uint id)
    {
      uint lengthOnDisk0 = lengthOnDisk(id);
      uint length;
      if (lengthOnDisk0 == 0)
      {
        length = cache_[id].size();
      } else if (id % numChains() == 0)
      {
        length = lengthOnDisk0 + cache_[id].size() - 1;
      } else
      {
        length = lengthOnDisk0;
      }
      return length;
    }

    bool ChainArray::append(uint id, const State& proposedState)
    {
      State last = cache_[id].back();
      bool accepted = acceptProposal(proposedState, last, beta_[id]);

      if (accepted)
        cache_[id].push_back(proposedState);
      else
        cache_[id].push_back(last);

      cache_[id].back().accepted = accepted;
      if (cache_[id].size() == cacheLength_)
        flushCache(id);

      return accepted;
    }

    void ChainArray::initialise(uint id, const State& s)
    {
      cache_[id].push_back(s);
      cache_[id].back().accepted = true;
      if (cache_[id].size() == cacheLength_)
        flushCache(id);
    }

    void ChainArray::recoverFromCache(uint id)
    {
      uint stack = id / numChains();
      uint chain = id % numChains();
      VLOG(1) << "Recovering stack " << stack << " chain " << chain << " from cache:";
      beta_[id] = internal::doubleFromDb(db_.get(internal::toDbString(id, internal::DbEntryType::BETA)));
      sigma_[id] = internal::doubleFromDb(db_.get(internal::toDbString(id, internal::DbEntryType::SIGMA)));
      uint len = lengthOnDisk(id);
      VLOG(1) << "Has length " << len;
      VLOG(1) << "Current cache length: " << cache_[id].size();
      if (len > 0)
      {
        cache_[id].push_back(stateFromDisk(id, len - 1));
      }
    }

    void ChainArray::flushCache(uint id)
    {
      leveldb::WriteBatch batch;
      uint diskLength = lengthOnDisk(id);
      uint cacheLength = cache_[id].size();
      State lastState = cache_[id].back();

      // Don't put the front state in
      if (id % numChains() == 0)
      {
        for (uint i = 1; i < cacheLength; i++)
        {
          uint index = diskLength + i - 1;
          batch.Put(internal::toDbString(id, index, internal::DbEntryType::STATE), comms::serialise(cache_[id][i]));
        }
        uint newLength = diskLength + cacheLength - 1; // no front state
        batch.Put(internal::toDbString(id, internal::DbEntryType::LENGTH), leveldb::Slice((char*) &newLength, sizeof(uint) / sizeof(char)));
        VLOG(3) << "Flushing cache of chain " << id << ". new length: " << newLength;
      } else
      {
        uint index = 0;
        batch.Put(internal::toDbString(id, index, internal::DbEntryType::STATE), comms::serialise(cache_[id][cacheLength - 1]));
        VLOG(3) << "Overwriting high temperature state of chain " << id;
        uint newLength = 1; // no front state
        batch.Put(toDbString(id, internal::DbEntryType::LENGTH), leveldb::Slice((char*) &newLength, sizeof(uint) / sizeof(char)));
      }

      // Update sigma and beta
      double sigma = sigma_[id];
      double beta = beta_[id];
      batch.Put(internal::toDbString(id, internal::DbEntryType::SIGMA), leveldb::Slice((char*) &sigma, sizeof(double) / sizeof(char)));
      batch.Put(internal::toDbString(id, internal::DbEntryType::BETA), leveldb::Slice((char*) &beta, sizeof(double) / sizeof(char)));

      // Write the batch
      db_.batch(batch);

      // Re-initialise the cache
      cache_[id].clear();
      cache_[id].push_back(lastState);
    }

    State ChainArray::lastState(uint id)
    {
      return cache_[id].back();
    }

    State ChainArray::stateFromDisk(uint id, uint idx)
    {
      uint dlen = lengthOnDisk(id);
      CHECK(idx < dlen) << "Can't access state " << idx << " in chain " << id << " from disk when " << dlen << " states stored on disk";
      State s;
      std::string data = db_.get(toDbString(id, idx, internal::DbEntryType::STATE));
      comms::unserialise(data, s);
      return s;
    }

    State ChainArray::stateFromCache(uint id, uint idx)
    {
      CHECK(cache_[id].size() > 0) << "Can't access state " << idx << " in chain " << id << " from cache when cache empty!";
      uint dlen = lengthOnDisk(id);
      uint cacheIdx = idx - dlen;
      CHECK(cacheIdx < cache_[id].size()) << "Can't access state " << idx << " in chain " << id << ": index beyond cache boundary";
      return cache_[id][cacheIdx];
    }

    State ChainArray::state(uint id, uint idx)
    {
      uint dlen = lengthOnDisk(id);
      uint cacheSize = cache_[id].size();
      if (idx < dlen || (idx == dlen && cacheSize == 0))
        return stateFromDisk(id, idx);
      else
        return stateFromCache(id, idx);
    }

    std::vector<State> ChainArray::states(uint id)
    {
      uint len = length(id);
      std::vector<State> v(len);
      for (uint i = 0; i < len; i++)
      {
        v[i] = state(id, i);
      }
      return v;
    }

    bool ChainArray::swap(uint id1, uint id2)
    {
      State& state1 = cache_[id1].back();
      State& state2 = cache_[id2].back();
      bool swapped = acceptSwap(state1, state2, beta_[id1], beta_[id2]);
      State tempState;
      if (swapped)
      {
        tempState = state1;
        state1.sample = state2.sample;
        state1.energy = state2.energy;
        state1.accepted = state2.accepted;
        state1.swapType = SwapType::Accept;
        state2.sample = tempState.sample;
        state2.energy = tempState.energy;
        state2.accepted = tempState.accepted;
        state2.swapType = SwapType::Accept;
      } else
      {
        state1.swapType = SwapType::Reject;
        state2.swapType = SwapType::Reject;
      }
      return swapped;
    }

    double ChainArray::sigma(uint id) const
    {
      return sigma_[id];
    }

    double ChainArray::beta(uint id) const
    {
      return beta_[id];
    }

    void ChainArray::setSigma(uint id, double sigma)
    {
      sigma_[id] = sigma;
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

  } // namespace mcmc
} // namespace obsidian
