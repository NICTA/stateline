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
        Sampler(const ProblemInstance& problem, const SamplerSettings& settings,
            const std::vector<Eigen::VectorXd>& initialStates,
            const std::vector<Eigen::VectorXd>& initialSigmas,
            const std::vector<double>& initialBetas);
      
        // Recover!
        Sampler(const ProblemInstance& problem, const SamplerSettings& settings);
      
        std::pair<uint, State> step(const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& betas);

        void flush();

        const ChainArray &chains() const;

    private:
      void propose(uint id);

      void unlock(uint id);

      void start();

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
  }
}
