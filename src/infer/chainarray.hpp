//!
//! Contains the interface for MCMC chains.
//!
//! \file infer/chainarray.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "db/db.hpp"
#include "mcmctypes.hpp"

namespace stateline
{
  namespace mcmc
  {
    //! Manager for all the states and handle reading / writing from / to databse.
    //!
    //! \section id The chain ID used by MCMC sampler.
    //!
    //! This class works with Ids:
    //! \code
    //! if nStacks = 2, nChains = 4
    //! id 0 = stack 1 chain 1 // lowest temperature chain of stack 1
    //! id 1 = stack 1 chain 2 // high temperature chain of stack 1
    //! id 2 = stack 1 chain 3 // high temperature chain of stack 1
    //! id 3 = stack 1 chain 4 // highest temperature chain of stack 1
    //! id 4 = stack 2 chain 1 // lowest temperature chain of stack 2
    //! id 5 = stack 2 chain 2 // high temperature chain of stack 2
    //! id 6 = stack 2 chain 3 // high temperature chain of stack 2
    //! id 7 = stack 2 chain 4 // highest temperature chain of stack 2
    //! \endcode
    //!
    class ChainArray
    {
      public:
        //! Create a chain array.
        //! 
        //! \param nStacks The number of stacks. Each stack have the same temperature sequence.
        //! \param nChains The number of chains in each stack.
        //! \param tempFactor The ratio between the temperatures of consecutive chains in a stack.
        //! \param initialSigma The initial step sizes of the chains.
        //! \param sigmaFactor The ratio between the temperatures of consectuive chains in a stack.
        //! \param db The database to store chain data.
        //! \param cacheLength The size of the memory cache used to store the chains.
        //! \param recover If set to true, chain data will be recovered from the database
        //                 otherwise, the MCMC will start from the beginning.
        //!
        ChainArray(uint nStacks, uint nChains, double tempFactor, double initialSigma,
            double sigmaFactor, db::Database& db, uint cacheLength, bool recover);

        //! Get the length of a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The number of elements in the chain.
        //!
        uint length(uint id);

        //! Append a state to a chain.
        //! 
        //! \param id The id of the chain (see \ref id).
        //! \param proposedState The new state to append.
        //! \return Whether the state accepted or rejected (in which case last state is reappended).
        //!
        bool append(uint id, const State& proposedState);

        //! Initialise a chain (by definitely accepting a new state).
        //!
        //! \param id The id of the chain (see \ref id).
        //! \param state the new state to append
        //!
        void initialise(uint id, const State& state);

        //! Return the last state from a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The most recent state in the chain.
        //!
        State lastState(uint id);

        //! Return a particular state.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \param index The index of the state (0 for first state).
        //! \return The most recent state in the chain.
        //!
        State state(uint id, uint index);

        //! Return all states from a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The most recent state in the chain.
        //!
        std::vector<State> states(uint id);

        //! Attempt to swap the states in two different chains.
        //!
        //! \param id1 The id of the first chain (see \ref id).
        //! \param id2 The id of the second chain (see \ref id).
        //! \return Whether the swap was successful.
        //!
        bool swap(uint id1, uint id2);

        //! Get the proposal width of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \return The proposal width of the chain.
        //!
        double sigma(uint id) const;

        //! Set the proposal width of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \param sigma The new proposal width value.
        //!
        void setSigma(uint id, double sigma);

        //! Get the inverse temperature of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \return The inverse temperature of the chain.
        //!
        double beta(uint id) const;

        //! Set the inverse temperature of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \param beta The new inverse temperature of the chain.
        //!
        void setBeta(uint id, double beta);

        //! Get the number of stacks.
        //!
        //! \return The number of stacks in the chain array.
        //!
        uint numStacks() const;

        //! Get the number of chains per stack.
        //!
        //! \return The number of chains in each stack of the chain array.
        //!
        uint numChains() const;

        //! Get the total number of chains in every stack.
        //!
        //! \return The total number of chains in the chain array.
        //!
        uint numTotalChains() const;

        //! Forcibly flush the cache for a particular chain.
        //!
        //! \param id The id of the chain to flush.
        //!
        void flushCache(uint id);

        //! Recover a particular chain from cached.
        //!
        //! \param id The id of the chain to recover.
        //!
        void recoverFromCache(uint id);

      private:
        uint lengthOnDisk(uint id);
        void setLengthOnDisk(uint id, uint length);

        State stateFromDisk(uint id, uint index);
        State stateFromCache(uint id, uint index);

        uint nstacks_;
        uint nchains_;
        uint cacheLength_;
        std::vector<double> beta_;
        std::vector<double> sigma_;
        std::vector<std::vector<State>> cache_;
        db::Database& db_;
    };

    namespace internal
    {
      //! Represents the different types of entries that the database can contain.
      //!
      enum class DbEntryType
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
      //! \param id The id of the chain.
      //! \param index A value representing the index of the value. This is useful
      //!              for time series or array data (such as the chain states).
      //! \param t The type of entry.
      //! \return A string representing the database key that is to store this entry.
      //!
      std::string toDbString(uint id, uint index, const DbEntryType& t);

      //! Get the key string representing a database entry of a particular chain. 
      //!
      //! \param id The id of the chain.
      //! \param t The type of entry.
      //! \return A string representing the database key that is to store this entry.
      //!
      std::string toDbString(uint id, const DbEntryType& t);

      //! Convert a database string value into a floating point value.
      //!
      //! \param data The string data from database.
      //! \return A floating point value that the string data represents.
      //!
      double doubleFromDb(const std::string& data);

      //! Convert a database string value into an unsigned integer value. 
      //!
      //! \param data The string data from database.
      //! \return An unsigned integer value that the string data represents.
      //!
      uint uintFromDb(const std::string& data);
    } // namespace internal

  } // namespace mcmc
} // namespace obsidian
