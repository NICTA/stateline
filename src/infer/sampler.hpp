//!
//! Contains the interface of an MCMC sampler.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "infer/mcmc.hpp"

namespace stateline
{
  namespace mcmc
  {
    class Sampler
    {
      public:
        Sampler(WorkerInterface& workerInterface, 
                ChainArray& chainArray,
                const ProposalFunction& propFn,
                const SamplerSettings& settings);
      
        std::pair<uint, State> step(const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& betas);

        void flush();

      private:

        void propose(uint id);

        void unlock(uint id);

        WorkerInterface& workerInterface_;
        // The MCMC chain wrapper
        ChainArray& chains_;
        
        ProposalFunction propFn_;
        
        SamplerSettings settings_;

        // convenience variables
        const uint nstacks_;
        const uint nchains_;

        // The proposed states in the process of being computed
        std::vector<Eigen::VectorXd> propStates_;

        // How many jobs haven't been retrieved?
        uint numOutstandingJobs_;

        // Whether a chain is locked. A locked chain will wait for any outstanding
        // job results and propagate the lock.
        std::vector<bool> locked_;

    };
  }
}
