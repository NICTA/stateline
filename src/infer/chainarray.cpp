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
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace boost
{
  namespace serialization
  {
    template<class Archive, typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
    void serialize(Archive &ar, 
        Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> &t, 
        const unsigned int version) 
    {
      // Adapted from http://stackoverflow.com/questions/12851126/serializing-eigens-matrix-using-boost-serialization
      for(std::size_t i = 0; i < t.size(); i++)
      {
        ar & t.data()[i];
      }
    }

    template <class Archive>
    void serialize(Archive &ar, ::stateline::mcmc::State &s, const unsigned int version) {
      ar & s.sample & s.energy & s.beta & s.accepted & s.swapType;
    }
  }
}

namespace stateline
{
  namespace mcmc
  {
    namespace detail
    {
      //! Represents the different types of entries that the database can contain.
      //!
      enum DbEntryType : std::int32_t
      {
        //! Indicates that the database entry is the state vector of a chain.
        STATE,

        //! Indicates that the database entry is the length of a chain.
        LENGTH,
        
        //! Indicates that the database entry is the step size of a chain.
        SIGMA,
        
        //! Indicates that the database entry is the inverse temperature of a chain.
        BETA
      };

      //! Get the key string representing a database entry of a particular chain.
      //!
      //! \tparam t The type of entry.
      //! \param id The id of the chain.
      //! \param index A value representing the index of the value. This is useful
      //!              for time series or array data (such as the chain states).
      //!              Set this to 0 if only one value of this type exists for
      //!              each chain.
      //! \return A string representing the database key that is to store this entry.
      //!
      template <std::int32_t EntryType>
      std::string toDbKeyString(std::uint32_t id, std::uint32_t index)
      {
        // Convert (type, id, index) into a string key
        std::uint32_t idx[3] = { EntryType, id, index };
        return std::string((char *)&idx[0], sizeof(idx));
      }

      template <std::int32_t EntryType, class T>
      void putToBatch(leveldb::WriteBatch &batch, std::uint32_t id, std::uint32_t index,
          T value)
      {
        std::cout << "BATCH: " << id << " " << index << " " << value << std::endl;
        // Write the given value to the batch buffer
        batch.Put(toDbKeyString<EntryType>(id, index),
            leveldb::Slice((char *)&value, sizeof(T)));
      }

      template <std::int32_t EntryType>
      void putToBatch(leveldb::WriteBatch &batch, std::uint32_t id, std::uint32_t index,
          std::string value)
      {
        std::cout << "STRING BATCH: " << id << " " << index << " " << value << std::endl;
        // Write the given value to the batch buffer
        batch.Put(toDbKeyString<EntryType>(id, index),
            leveldb::Slice(value.c_str(), sizeof(char) * value.length()));
      }

      template <std::int32_t EntryType, class T>
      T getFromDb(db::Database &db, std::uint32_t id, std::uint32_t index)
      {
        // Read the given value to the database
        std::string result = db.get(toDbKeyString<EntryType>(id, index));
        std::cout << "result len: " << result.size() << std::endl;

        // Convert it to the data type that we want.
        return *((T *) &result[0]);
      }

      template <std::int32_t EntryType>
      std::string getFromDb(db::Database &db, std::uint32_t id, std::uint32_t index)
      {
        // Read the given value to the database
        std::string result = db.get(toDbKeyString<EntryType>(id, index));
        std::cout << "string result len: " << result.size() << std::endl;
        return result;
      }

      std::string serialiseState(const State &state)
      {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << state;
        std::cout << "serialised as " << ss.str() << std::endl;
        return ss.str();
      }

      State unserialiseState(const std::string &str)
      {
        std::cout << "unserialising " << str << std::endl;
        std::stringstream ss;
        ss << str;

        State state;
        boost::archive::text_iarchive ia(ss);
        ia >> state;

        std::cout << "unserialised " << state.energy << std::endl;
        return state;
      }
    } // namespace detail

    ChainArray::ChainArray(uint nStacks, uint nChains, double tempFactor,
        double initialSigma, double sigmaFactor, const DBSettings& d, uint cacheLength)
        : nstacks_(nStacks),
          nchains_(nChains),
          cacheLength_(cacheLength),
          beta_(nStacks * nChains),
          sigma_(nStacks * nChains),
          cache_(nStacks * nChains),
          db_(d)
    {
      // Reserve the cache
      for (auto& c : cache_)
        c.reserve(cacheLength);

      // Set ourselves up in the last state
      if (d.recover)
      {
        LOG(INFO)<< "Recovering chains...";
        for(uint id = 0; id < nChains * nStacks; id++)
        {
          recoverFromDisk(id);
        }
      }
      else
      {
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
            detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, 0);
            detail::putToBatch<detail::SIGMA>(batch, id, 0, sigma);
            detail::putToBatch<detail::BETA>(batch, id, 0, beta);
          }
        }

        db_.batch(batch);
      }
    }

    uint ChainArray::lengthOnDisk(uint id)
    {
      return detail::getFromDb<detail::LENGTH, std::uint32_t>(db_, id, 0);
    }

    uint ChainArray::length(uint id)
    {
      uint lengthOnDisk0 = lengthOnDisk(id);
      uint length;

      if (lengthOnDisk0 == 0)
      {
        length = cache_[id].size();
      }
      else if (id % numChains() == 0)
      {
        length = lengthOnDisk0 + cache_[id].size() - 1;
      }
      else
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
        flushToDisk(id);

      return accepted;
    }

    void ChainArray::initialise(uint id, const State& s)
    {
      cache_[id].push_back(s);
      cache_[id].back().accepted = true;
      if (cache_[id].size() == cacheLength_)
        flushToDisk(id);
    }

    void ChainArray::recoverFromDisk(uint id)
    {
      uint stack = id / numChains();
      uint chain = id % numChains();

      // Recover beta and sigma
      VLOG(1) << "Recovering stack " << stack << " chain " << chain << " from disk:";
      beta_[id] = detail::getFromDb<detail::BETA, double>(db_, id, 0);
      sigma_[id] = detail::getFromDb<detail::SIGMA, double>(db_, id, 0);

      uint len = lengthOnDisk(id);
      VLOG(1) << "Has length " << len;
      VLOG(1) << "Current cache length: " << cache_[id].size();
      if (len > 0)
      {
        // Recover only the newest state
        cache_[id].push_back(stateFromDisk(id, len - 1));
      }
    }

    void ChainArray::flushToDisk(uint id)
    {
      leveldb::WriteBatch batch;
      uint diskLength = lengthOnDisk(id);
      uint cacheLength = cache_[id].size();
      State lastState = cache_[id].back();

      // Don't put the newest state in
      if (id % numChains() == 0)
      {
        std::cout << "Flushing size " << cacheLength << " with " << diskLength << " already on disk" << std::endl;
        for (uint i = 0; i < cacheLength - 1; i++)
        {
          uint index = diskLength + i;
          detail::putToBatch<detail::STATE>(batch, id, index,
              detail::serialiseState(cache_[id][i]));
        }

        uint newLength = diskLength + cacheLength - 1; // don't count newest state
        VLOG(3) << "Flushing cache of chain " << id << ". new length: " << newLength;
        detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, newLength);
      }
      else
      {
        VLOG(3) << "Overwriting high temperature state of chain " << id;
        detail::putToBatch<detail::STATE>(batch, id, 0,
          detail::serialiseState(cache_[id][cacheLength - 1]));
        detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, 1);
      }

      // Update sigma and beta
      detail::putToBatch<detail::SIGMA>(batch, id, 0, sigma_[id]);
      detail::putToBatch<detail::BETA>(batch, id, 0, beta_[id]);

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

      // Read the serialised state from disk
      std::string state = detail::getFromDb<detail::STATE>(db_, id, idx);

      return detail::unserialiseState(state);
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

      // Determine if we accept this swap
      bool swapped = acceptSwap(state1, state2, beta_[id1], beta_[id2]);

      if (swapped)
      {
        std::swap(state1, state2);
        state1.swapType = SwapType::Accept;
        state2.swapType = SwapType::Accept;
      }
      else
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
