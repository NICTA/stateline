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
      // Adapted from http://stackoverflow.com/questions/12580579/how-to-use-boostserialization-to-save-eigenmatrix/12618789#12618789
      std::size_t rows = t.rows(), cols = t.cols();
      ar & rows & cols;

      if ((uint)(rows * cols) != t.size())
      {
        // Allocate memory if necessary
        t.resize(rows, cols);
      }

      for(std::size_t i = 0; i < (std::size_t)t.size(); i++)
      {
        ar & t.data()[i];
      }
    }

    template <class Archive>
    void serialize(Archive &ar, ::stateline::mcmc::State &s, const unsigned int version) {
      ar & s.sample & s.energy & s.sigma & s.beta & s.accepted & s.swapType;
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
      enum DBEntryType : std::int32_t
      {
        //! Indicates the number of stacks.
        NSTACKS,

        //! Indicates the number of chains.
        NCHAINS,

        //! Represents the state vector of a chain.
        STATE,

        //! Represents the length of a chain.
        LENGTH,
        
        //! Represents the step size of a chain.
        SIGMA,
        
        //! Represents the inverse temperature of a chain.
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
      std::string toDBKeyString(std::uint32_t id, std::uint32_t index)
      {
        // Convert (type, id, index) into a string key
        std::uint32_t idx[3] = { EntryType, id, index };
        return std::string((char *)&idx[0], sizeof(idx));
      }

      template <std::int32_t EntryType, class T>
      void putToBatch(leveldb::WriteBatch &batch, std::uint32_t id, std::uint32_t index,
          T value)
      {
        // Write the given value to the batch buffer
        batch.Put(toDBKeyString<EntryType>(id, index),
            leveldb::Slice((char *)&value, sizeof(T)));
      }

      template <std::int32_t EntryType>
      void putToBatch(leveldb::WriteBatch &batch, std::uint32_t id, std::uint32_t index,
          std::string value)
      {
        // Write the given value to the batch buffer
        batch.Put(toDBKeyString<EntryType>(id, index),
            leveldb::Slice(value.c_str(), sizeof(char) * value.length()));
      }

      template <std::int32_t EntryType>
      std::string getFromDb(db::Database &db, std::uint32_t id = 0, std::uint32_t index = 0)
      {
        // Read the given value to the database
        std::string result = db.get(toDBKeyString<EntryType>(id, index));
        return result;
      }

      template <std::int32_t EntryType, class T>
      T getFromDb(db::Database &db, std::uint32_t id = 0, std::uint32_t index = 0)
      {
        // Read the given value to the database
        std::string result = db.get(toDBKeyString<EntryType>(id, index));

        // Convert it to the data type that we want.
        return *((T *) &result[0]);
      }

      template <class T>
      std::string archiveString(const T &state)
      {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << state;
        return ss.str();
      }

      template <class T>
      T unarchiveString(const std::string &str)
      {
        std::stringstream ss;
        ss << str;

        T obj;
        boost::archive::text_iarchive ia(ss);
        ia >> obj;

        return obj;
      }
    } // namespace detail

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

    ChainArray::ChainArray(const DBSettings &d, uint cacheLength)
      : db_(d, true),
        cacheLength_(cacheLength)
    {
      // Recover the dimensions of the chain array
      nstacks_ = detail::getFromDb<detail::NSTACKS, std::uint32_t>(db_);
      nchains_ = detail::getFromDb<detail::NCHAINS, std::uint32_t>(db_);

      // Recover each of the chains
      for (uint id = 0; id < nstacks_ * nchains_; ++id)
      {
        // Recover beta and sigma
        VLOG(1) << "Recovering chain " << id << " from disk.";
        beta_.push_back(detail::getFromDb<detail::BETA, double>(db_, id));
        sigma_.push_back(detail::unarchiveString<Eigen::VectorXd>(detail::getFromDb<detail::SIGMA>(db_, id)));

        // Recover the newest state
        uint len = lengthOnDisk(id);
        VLOG(1) << "Has length " << len;
        if (len > 0)
        {
          cache_.push_back({ stateFromDisk(id, len - 1) });
        }
        else
        {
          cache_.push_back({});
        }

        // Reserve the cache
        cache_.back().reserve(cacheLength);
      }
    }

    ChainArray::ChainArray(uint nStacks, uint nChains,
        const std::vector<Eigen::VectorXd>& initialSigmas,
        const std::vector<double>& initialBetas, const DBSettings& d, uint cacheLength)
        : db_(d, false),
          nstacks_(nStacks),
          nchains_(nChains),
          cacheLength_(cacheLength),
          beta_(nStacks * nChains),
          sigma_(nStacks * nChains),
          cache_(nStacks * nChains)
    {
      // Reserve the cache
      for (auto& c : cache_)
        c.reserve(cacheLength);

      // Initialise the database
      leveldb::WriteBatch batch;

      detail::putToBatch<detail::NSTACKS, std::uint32_t>(batch, 0, 0, nStacks);
      detail::putToBatch<detail::NCHAINS, std::uint32_t>(batch, 0, 0, nChains);

      for (uint i = 0; i < nStacks; i++)
      {
        for (uint j = 0; j < nChains; j++)
        {
          uint id = i * nChains + j;

          beta_[id] = initialBetas[id];
          sigma_[id] = initialSigmas[id];

          // All chains start off with length 0
          detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, 0);
          detail::putToBatch<detail::SIGMA>(batch, id, 0, detail::archiveString(sigma_[id]));
          detail::putToBatch<detail::BETA>(batch, id, 0, beta_[id]);
        }
      }

      db_.put(batch);
    }

    ChainArray::ChainArray(ChainArray&& other)
      : db_(std::move(db_)), nstacks_(other.nstacks_), nchains_(other.nchains_),
        cacheLength_(other.cacheLength_), beta_(std::move(other.beta_)), sigma_(std::move(other.sigma_)),
        cache_(std::move(other.cache_))
    {
    }

    uint ChainArray::lengthOnDisk(uint id) const
    {
      return detail::getFromDb<detail::LENGTH, std::uint32_t>(db_, id);
    }

    uint ChainArray::length(uint id) const
    {
      uint lengthOnDisk0 = lengthOnDisk(id);
      uint length;

      if (lengthOnDisk0 == 0)
      {
        length = cache_[id].size();
      }
      else if (chainIndex(id) == 0)
      {
        length = lengthOnDisk0 + cache_[id].size() - 1;
      }
      else
      {
        length = lengthOnDisk0;
      }

      return length;
    }

    bool ChainArray::append(uint id, const Eigen::VectorXd& sample, double energy)
    {
      State newState = { sample, energy, sigma_[id], beta_[id] };
      State last = cache_[id].back();

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

    void ChainArray::initialise(uint id, const Eigen::VectorXd& sample, double energy)
    {
      cache_[id].push_back({ sample, energy, sigma_[id], beta_[id] });
      cache_[id].back().accepted = true;
      cache_[id].back().swapType = SwapType::NoAttempt;
      if (cache_[id].size() == cacheLength_)
        flushToDisk(id);
    }

    void ChainArray::flushToDisk(uint id)
    {
      leveldb::WriteBatch batch;
      uint diskLength = lengthOnDisk(id);
      uint cacheLength = cache_[id].size();
      State lastState = cache_[id].back();

      // Don't put the newest state in
      if (chainIndex(id) == 0)
      {
        for (uint i = 0; i < cacheLength - 1; i++)
        {
          uint index = diskLength + i;
          detail::putToBatch<detail::STATE>(batch, id, index,
              detail::archiveString(cache_[id][i]));
        }

        uint newLength = diskLength + cacheLength - 1; // don't count newest state
        VLOG(3) << "Flushing cache of chain " << id << ". new length: " << newLength;
        detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, newLength);
      }
      else
      {
        VLOG(3) << "Overwriting high temperature state of chain " << id;
        detail::putToBatch<detail::STATE>(batch, id, 0,
          detail::archiveString(cache_[id][cacheLength - 1]));
        detail::putToBatch<detail::LENGTH, std::uint32_t>(batch, id, 0, 1);
      }

      // Update sigma and beta
      detail::putToBatch<detail::SIGMA>(batch, id, 0, detail::archiveString(sigma_[id]));
      detail::putToBatch<detail::BETA>(batch, id, 0, beta_[id]);

      // Write the batch
      db_.put(batch);

      // Re-initialise the cache
      cache_[id].clear();
      cache_[id].push_back(lastState);
    }

    State ChainArray::lastState(uint id) const
    {
      return cache_[id].back();
    }

    State ChainArray::stateFromDisk(uint id, uint idx) const
    {
      uint dlen = lengthOnDisk(id);
      CHECK(idx < dlen) << "Can't access state " << idx << " in chain " << id << " from disk when " << dlen << " states stored on disk";

      // Read the serialised state from disk
      std::string state = detail::getFromDb<detail::STATE>(db_, id, idx);

      return detail::unarchiveString<State>(state);
    }

    State ChainArray::stateFromCache(uint id, uint idx) const
    {
      CHECK(cache_[id].size() > 0) << "Can't access state " << idx << " in chain " << id << " from cache when cache empty!";

      uint dlen = lengthOnDisk(id);
      uint cacheIdx = idx - dlen;

      CHECK(cacheIdx < cache_[id].size()) << "Can't access state " << idx << " in chain " << id << ": index beyond cache boundary";

      return cache_[id][cacheIdx];
    }

    State ChainArray::state(uint id, uint idx) const
    {
      uint dlen = lengthOnDisk(id);
      uint cacheSize = cache_[id].size();
      if (idx < dlen || (idx == dlen && cacheSize == 0))
        return stateFromDisk(id, idx);
      else
        return stateFromCache(id, idx);
    }

    std::vector<State> ChainArray::states(uint id) const
    {
      uint len = length(id);
      std::vector<State> v(len);
      for (uint i = 0; i < len; i++)
      {
        v[i] = state(id, i);
      }
      return v;
    }

    SwapType ChainArray::swap(uint id1, uint id2)
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

      return swapped ? SwapType::Accept : SwapType::Reject;
    }

    Eigen::VectorXd ChainArray::sigma(uint id) const
    {
      return sigma_[id];
    }

    void ChainArray::setSigma(uint id, const Eigen::VectorXd& sigma)
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
