//!
//! Settings for MCMC.
//!
//! \file infer/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

namespace stateline
{
  //! Settings for Markov Chain Monte Carlo simulations.
  //!
  struct MCMCSettings
  {
    //! The number of chains per stack.
    uint chains;

    //! The number of stacks used in the simulation.
    uint stacks;

    //! Maximum time the simulation should run for.
    uint wallTime;

    //! The number of samples between swap attempts.
    uint swapInterval;

    //! The number of samples from the beginning 
    uint adaptionLength;

    //! The number of samples kept in memory. The MCMC uses a sliding buffer
    //! which flushes to disk old samples when the cache is full.
    uint cacheLength;

    //! The initial proposal width factor. Initially, the proposal width of
    //! a chain will be initialSigmaFactor times the proposal width of the
    //! chain below (colder) than it.
    double initialSigmaFactor;
 
    //! The initial proposal width of the coldest chains.
    double proposalInitialSigma;

    //! The minimum proposal adaption factor allowed. This controls how much
    //! the proposal width can change in one adaption step.
    double proposalMinFactor;

    //! The maximum proposal adaption factor allowed. This controls how much
    //! the proposal width can change in one adaption step.
    double proposalMaxFactor;

    //! The optimal acceptance rate for any of the chains.
    double proposalOptimalAccept;

    //! The rate at which the proposal width changes when it adapts.
    double proposalAdaptRate;

    //! The number of samples between each attempt to adapt the proposal width.
    uint proposalAdaptInterval;

    //! The optimal swap rate for any of the chains.
    double betaOptimalSwapRate;

    //! The rate at which the inverse temperature changes when it adapts.
    double betaAdaptRate;

    //! The minimum beta adaption factor allowed. This controls how much
    //! the temperature can change in one adaption step.
    double betaMinFactor;

    //! The minimum beta adaption factor allowed. This controls how much
    //! the temperature can change in one adaption step.
    double betaMaxFactor;

    //! The number of samples between each attempt to adapt the temperature.
    uint betaAdaptInterval;

    //! The initial temperature ladder factor.
    double initialTempFactor;

    //! Empty settings with all parameters set to zero.
    static MCMCSettings Empty()
    {
      MCMCSettings settings = {};
      return settings;
    }
  };
}
