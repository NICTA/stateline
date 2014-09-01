//!
//! Contains the implementation of an MCMC sampler.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/sampler.hpp"

namespace stateline
{
  namespace mcmc
  {
    Sampler::Sampler(WorkerInterface& workerInterface, 
                     ChainArray& chainArray,
                     const ProposalFunction& propFn,
                     uint swapInterval)
      : workerInterface_(workerInterface),
        chains_(chainArray),
        propFn_(propFn),
        nstacks_(chains_.numStacks()),
        nchains_(chains_.numChains()),
        propStates_(nstacks_*nchains_),
        swapInterval_(swapInterval),
        numOutstandingJobs_(0),
        locked_(nstacks_ * nchains_, false)
    {
      // Start all the chains from hottest to coldest
      for (uint i = 0; i < chains_.numTotalChains(); i++)
      {
        uint c = chains_.numTotalChains() - i - 1;
        propose(c);
      }
    }
    
    std::pair<uint, State> Sampler::step(const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& betas)
    {
      // Listen for replies. As soon as a new state comes back,
      // add it to the corresponding chain, and submit a new proposed state
      uint id;
      double energy;
      // Wait a for reply
      try
      {
        std::tie(id, energy) = workerInterface_.retrieve();
      }
      catch (...)
      {
        VLOG(3) << "Comms error -- probably shutting down";
      }

      numOutstandingJobs_--;

      // Update the parameters for this id
      chains_.setSigma(id, sigmas[id]);
      chains_.setBeta(id, betas[id]);

      // Handle the new proposal and add a new state to the chain
      chains_.append(id, propStates_[id], energy);

      // Check if this chain was locked. If it was locked, it means that
      // the chain above (hotter) locked it so that it can swap with it
      if (locked_[id])
      {
        // Try swapping this chain with the one above it
        chains_.swap(id, id + 1);
        // Unlock this chain, propgating the lock downwards
        unlock(id);
      }
      else if (chains_.isHottestInStack(id) && chains_.length(id) % swapInterval_ == 0 && chains_.numChains() > 1)
      {
        // The hottest chain is ready to swap. Lock the next chain
        // to prevent it from proposing any more
        locked_[id - 1] = true;
      }
      else
      {
        // This chain is not locked, so we can propose
        try
        {
          propose(id);
        }
        catch (...)
        {
          VLOG(3) << "Comms error -- probably shutting down";
        }
      }

      return {id, chains_.lastState(id)};
    }

    void Sampler::flush()
    {
      // Retrieve all outstanding job results.
      while (numOutstandingJobs_--)
      {
        uint id;
        double energy;
        std::tie(id, energy) = workerInterface_.retrieve();
        chains_.append(id, propStates_[id], energy);
      }

      // Manually flush any chain states that are in memory to disk
      for (uint i = 0; i < chains_.numTotalChains(); i++)
        chains_.flushToDisk(i);
    }

    void Sampler::propose(uint id)
    {
      propStates_[id] = propFn_(id, chains_);
      workerInterface_.submit(id, propStates_[id]);
      numOutstandingJobs_++;
    }

    void Sampler::unlock(uint id)
    {
      // Unlock this chain
      locked_[id] = false;

      // The hotter chain no longer has to wait for this chain, so
      // it can propose new state
      propose(id + 1);

      // Check if this was the coldest chain
      if (id % chains_.numChains() != 0)
      {
        // Lock the chain that is below (colder) than this.
        locked_[id - 1] = true;
      }
      else
      {
        // This is the coldest chain and there is no one to swap with
        propose(id);
      }
    }

  
  }
}
