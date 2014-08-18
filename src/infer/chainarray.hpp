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
#include "infer/state.hpp"

namespace stateline
{
  namespace mcmc
  {
    //! Manager for all the states and handle reading / writing from / to database.
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
        //! Recover a chain array from disk.
        //! 
        //! \param d The database settings used to store chain data.
        //!
        ChainArray(const DBSettings &d, uint cacheLength);

        //! Create a chain array.
        //! 
        //! \param nStacks The number of stacks. Each stack have the same temperature sequence.
        //! \param nChains The number of chains in each stack.
        //! \param tempFactor The ratio between the temperatures of consecutive chains in a stack.
        //! \param initialSigma The initial step sizes of the chains.
        //! \param sigmaFactor The ratio between the temperatures of consectuive chains in a stack.
        //! \param d The database settings used to store chain data.
        //! \param cacheLength The size of the memory cache used to store the chains.
        //!
        ChainArray(uint nStacks, uint nChains, const std::vector<Eigen::VectorXd>& initialSigmas,
            const std::vector<double>& initialBetas, const DBSettings &d, uint cacheLength);

        // Move constructor only
        ChainArray(ChainArray&& other);

        //! Get the length of a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The number of elements in the chain.
        //!
        uint length(uint id) const;

        //! Append a state to a chain.
        //! 
        //! \param id The id of the chain (see \ref id).
        //! \param proposedState The new state to append.
        //! \return Whether the state accepted or rejected (in which case last state is reappended).
        //!
        bool append(uint id, const Eigen::VectorXd& sample, double energy);

        //! Initialise a chain (by definitely accepting a new state).
        //!
        //! \param id The id of the chain (see \ref id).
        //! \param state the new state to append
        //!
        void initialise(uint id, const Eigen::VectorXd& sample, double energy);

        //! Forcibly flush the cache for a particular chain to disk.
        //!
        //! \param id The id of the chain to flush.
        //!
        void flushToDisk(uint id);

        //! Return the last state from a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The most recent state in the chain.
        //!
        State lastState(uint id) const;

        //! Return a particular state.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \param index The index of the state (0 for first state).
        //! \return The most recent state in the chain.
        //!
        State state(uint id, uint index) const;

        //! Return all states from a chain.
        //!
        //! \param id The id of the chain (see \ref id).
        //! \return The most recent state in the chain.
        //!
        std::vector<State> states(uint id) const;

        //! Attempt to swap the states in two different chains.
        //!
        //! \param id1 The id of the first chain (see \ref id).
        //! \param id2 The id of the second chain (see \ref id).
        //! \return Whether the swap was successful.
        //!
        SwapType swap(uint id1, uint id2);

        //! Get the proposal width of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \return The proposal width of the chain.
        //!
        Eigen::VectorXd sigma(uint id) const;

        //! Set the proposal width of a specific chain.
        //!
        //! \param id The id of the second chain (see \ref id).
        //! \param sigma The new proposal width value.
        //!
        void setSigma(uint id, const Eigen::VectorXd& sigma);

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

        uint stackIndex(uint id) const;

        uint chainIndex(uint id) const;

        bool isHottestInStack(uint id) const;

        bool isColdestInStack(uint id) const;

      private:
        uint lengthOnDisk(uint id) const;
        void setLengthOnDisk(uint id, uint length);

        State stateFromDisk(uint id, uint index) const;
        State stateFromCache(uint id, uint index) const;

        //! Recover a particular chain from disk.
        //!
        //! \param id The id of the chain to recover.
        //!
        void recoverFromDisk(uint id);

        mutable db::Database db_; // Mutable so that chain queries can be const
        uint nstacks_;
        uint nchains_;
        uint cacheLength_;
        std::vector<double> beta_;
        std::vector<Eigen::VectorXd> sigma_;
        std::vector<std::vector<State>> cache_;
    };

  } // namespace mcmc
} // namespace stateline
