//!
//! Contains the implementation of Monte Carlo Markov Chain simulations.
//!
//! \file infer/mcmc.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <limits>
#include <random>
#include <chrono>
#include <map>
#include <Eigen/Dense>
#include <iostream>
#include <glog/logging.h>

#include "db/db.hpp"
#include "comms/datatypes.hpp"
#include "comms/settings.hpp"
#include "comms/serial.hpp"
#include "infer/async.hpp"
#include "infer/settings.hpp"
#include "infer/chainarray.hpp"
#include "infer/diagnostics.hpp"
#include "app/serial.hpp"

namespace stateline
{
  namespace mcmc
  {
    using JobConstructFunction =
      std::function<std::vector<comms::JobData>(const Eigen::VectorXd&)>;

    using ResultLikelihoodFunction = 
      std::function<double(const std::vector<comms::ResultData>&)>;

    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const mcmc::ChainArray&)>;

    struct ProblemInstance
    {
      std::string globalJobSpecData;
      std::map<comms::JobID, std::string> perJobSpecData;

      JobConstructFunction jobConstructFn;
      ResultLikelihoodFunction resultLikelihoodFn;
      ProposalFunction proposalFn;
    };

    struct SamplerSettings
    {
      MCMCSettings mcmc;
      DBSettings db;
      DelegatorSettings del;
    };

    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x)
    {
      return {{ 0, "", serialise(x) }};
    }

    double singleJobLikelihood(const std::vector<comms::ResultData> &results)
    {
      return comms::detail::unserialise<double>(results[0].data);
    }

    class Sampler
    {
      public:
        // with initial states
        Sampler(const ProblemInstance& problem, const SamplerSettings& settings,
            const std::vector<Eigen::VectorXd>& initialStates,
            const std::vector<Eigen::VectorXd>& initialSigmas,
            const std::vector<double>& initialBetas)
          : problem_(problem),
            settings_(settings),
            nstacks_(settings.mcmc.stacks),
            nchains_(settings.mcmc.chains),
            chains_(nstacks_, nchains_, initialSigmas, initialBetas, settings.db, settings.mcmc.cacheLength),
            numOutstandingJobs_(0),
            locked_(settings.mcmc.stacks * settings.mcmc.chains, false),
            com_(problem.globalJobSpecData, problem.perJobSpecData, settings.del)
        {
          initialise(initialStates);
        }
      
        // Recover!
        Sampler(const ProblemInstance& problem, const SamplerSettings& settings)
          : problem_(problem),
          settings_(settings),
          nstacks_(settings.mcmc.stacks),
          nchains_(settings.mcmc.chains),
          chains_(settings.db, settings.mcmc.cacheLength),
          propStates_(nstacks_*nchains_),
          numOutstandingJobs_(0),
          locked_(settings.mcmc.stacks * settings.mcmc.chains, false),
          com_(problem.globalJobSpecData, problem.perJobSpecData, settings.del)
        {
        }
      
        std::pair<uint, State> step(const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& betas)
        {
          // Listen for replies. As soon as a new state comes back,
          // add it to the corresponding chain, and submit a new proposed state
          std::pair<uint, std::vector<comms::ResultData>> result;

          // Wait a for reply
          try
          {
            result = com_.retrieve();
          }
          catch (...)
          {
            VLOG(3) << "Comms error -- probably shutting down";
          }

          numOutstandingJobs_--;
          uint id = result.first;
          double energy = problem_.resultLikelihoodFn(result.second);

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
          else if (chains_.isHottestInStack(id) && chains_.length(id) % settings_.mcmc.swapInterval == 0 && chains_.numChains() > 1)
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

        void flush()
        {
          // Retrieve all outstanding job results.
          while (numOutstandingJobs_--)
          {
            auto result = com_.retrieve();
            uint id = result.first;
            double energy = problem_.resultLikelihoodFn(result.second);
            chains_.append(id, propStates_[id], energy);
          }

          // Manually flush any chain states that are in memory to disk
          for (uint i = 0; i < chains_.numTotalChains(); i++)
            chains_.flushToDisk(i);
        }

        const ChainArray &chains() const
        {
          return chains_;
        }

    private:
      void propose(uint id)
      {
        propStates_[id] = problem_.proposalFn(id, chains_);
        com_.submit(id, problem_.jobConstructFn(propStates_[id]));
        numOutstandingJobs_++;
      }

      void unlock(uint id)
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

      void initialise(const std::vector<Eigen::VectorXd>& initialStates)
      {
        // Initialise the chains if we're not recovering
        // Evaluate the initial states of the chains
        for (uint i = 0; i < chains_.numTotalChains(); i++)
        {
          propStates_[i] = initialStates[i];
          com_.submit(i, problem_.jobConstructFn(propStates_[i]));
        }

        // Retrieve the energies and temperatures for the initial states
        for (uint i = 0; i < chains_.numTotalChains(); i++)
        {
          auto result = com_.retrieve();
          uint id = result.first;
          double energy = problem_.resultLikelihoodFn(result.second);
          chains_.initialise(id, initialStates[i], energy); 
        }
        // Start all the chains from hottest to coldest
        for (uint i = 0; i < chains_.numTotalChains(); i++)
        {
          uint c = chains_.numTotalChains() - i - 1;
          propose(c);
        }
      }

      ProblemInstance problem_;
      SamplerSettings settings_;

      // convenience variables
      const uint nstacks_;
      const uint nchains_;

      // The MCMC chain wrapper
      ChainArray chains_;

      // The proposed states in the process of being computed
      std::vector<Eigen::VectorXd> propStates_;

      // How many jobs haven't been retrieved?
      uint numOutstandingJobs_;

      // Whether a chain is locked. A locked chain will wait for any outstanding
      // job results and propagate the lock.
      std::vector<bool> locked_;

      // The comms wrapper 
      AsyncCommunicator com_;
    };

  } // namespace mcmc 
}// namespace stateline
